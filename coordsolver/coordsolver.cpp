#include "coordsolver.h"

CoordSolver::CoordSolver()
{
}

CoordSolver::~CoordSolver()
{   
}

bool CoordSolver::loadParam(string coord_path,string param_name)
{
    YAML::Node config = YAML::LoadFile(coord_path);

    Eigen::MatrixXd mat_intrinsic(3, 3);
    Eigen::MatrixXd mat_coeff(1, 5);
    Eigen::MatrixXd mat_xyz_offset(1,3);
    Eigen::MatrixXd mat_angle_offset(1,2);

    //初始化内参矩阵
    auto read_vector = config[param_name]["Intrinsic"].as<vector<float>>();
    initMatrix(mat_intrinsic,read_vector);
    eigen2cv(mat_intrinsic,intrinsic);

    //初始化畸变矩阵
    read_vector = config[param_name]["Coeff"].as<vector<float>>();
    initMatrix(mat_coeff,read_vector);
    eigen2cv(mat_coeff,dis_coeff);

    read_vector = config[param_name]["xyz_offset"].as<vector<float>>();
    initMatrix(mat_xyz_offset,read_vector);
    xyz_offset = mat_xyz_offset.transpose();

    read_vector = config[param_name]["angle_offset"].as<vector<float>>();
    initMatrix(mat_angle_offset,read_vector);
    angle_offset = mat_angle_offset.transpose();

    return true;
}

Eigen::Vector3d CoordSolver::pnp(Point2f apex[4], int method=SOLVEPNP_ITERATIVE)
{
    std::vector<Point3d> points_world;
    std::vector<Point2f> points_pic(apex,apex + 4);

    points_world = {
        {-6.6,2.7,0},
        {-6.6,-2.7,0},
        {6.6,-2.7,0},
        {6.6,2.7,0}
    };

    Mat rvec;
    Mat tvec;

    solvePnP(points_world, points_pic, intrinsic, dis_coeff, rvec, tvec, false, method);

    Eigen::Vector3d result;
    cv2eigen(tvec, result);
    return result;
}


Eigen::Vector2d CoordSolver::getAngle(Eigen::Vector3d &xyz)
{
    auto xyz_offseted = staticCoordOffset(xyz);
    auto angle = calcYawPitch(xyz_offseted);
    auto angle_offseted = staticAngleOffset(angle);
    return angle_offseted;

}

inline Eigen::Vector3d CoordSolver::staticCoordOffset(Eigen::Vector3d &xyz)
{
    return xyz + xyz_offset;
}

inline Eigen::Vector2d CoordSolver::staticAngleOffset(Eigen::Vector2d &angle)
{
    return angle + angle_offset;
}

/**
 * @brief 计算目标Yaw,Pitch角度
 * @return Yaw与Pitch
*/
Eigen::Vector2d CoordSolver::calcYawPitch(Eigen::Vector3d &xyz)
{
    Eigen::Vector2d angle;

    //Yaw
    angle[0] = atan2(xyz[0], xyz[2]) * 180 / CV_PI;
    //Pitch
    angle[1] = atan2(xyz[1], sqrt(xyz[0] * xyz[0] + xyz[2] * xyz[2])) * 180 / CV_PI;

    return angle;
}

Eigen::Matrix3d eulerToRotationMatrix(Eigen::Vector3d &theta)
{
    Eigen::Matrix3d R_x;
    Eigen::Matrix3d R_y;
    Eigen::Matrix3d R_z;
    // Calculate rotation about x axis
    R_x <<
        1,       0,              0,
        0,       cos(theta[0]),   -sin(theta[0]),
        0,       sin(theta[0]),   cos(theta[0]);
    // Calculate rotation about y axis
    R_y <<
        cos(theta[1]),    0,      sin(theta[1]),
        0,               1,      0,
        -sin(theta[1]),   0,      cos(theta[1]);
    // Calculate rotation about z axis
    R_z <<
        cos(theta[2]),    -sin(theta[2]),      0,
        sin(theta[2]),    cos(theta[2]),       0,
        0,               0,                  1;
    // Combined rotation matrix
    return R_z * R_y * R_x;
}

