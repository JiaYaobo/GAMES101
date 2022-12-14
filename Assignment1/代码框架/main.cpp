#include "Triangle.hpp"
#include "rasterizer.hpp"
#include <Eigen/Eigen>
#include <iostream>
#include <opencv2/opencv.hpp>

constexpr double MY_PI = 3.1415926;


Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0], 0, 1, 0, -eye_pos[1], 0, 0, 1,
        -eye_pos[2], 0, 0, 0, 1;

    view = translate * view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

    float rad = rotation_angle * MY_PI / 180;

    model << cos(rad), -sin(rad), 0, 0,
                sin(rad), cos(rad), 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1;

    return model;
}

Eigen::Matrix4f get_model_matrix(Eigen::Vector3f n, float rotation_angle){
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f K = Eigen::Matrix4f::Identity();
    K << 0, -n[2], n[1], 0,
        n[2], 0, -n[0], 0,
        -n[1], n[0], 0, 0,
        0, 0, 0, 1;

    float rad = rotation_angle * MY_PI / 180;

    Eigen::Matrix4f R = Eigen::Matrix4f ::Identity() + sin(rad) * K + (1 - cos(rad)) * K * K;

    return R * model;
}

Eigen::Matrix4f get_rotation(Vector3f axis,float angle)
{
    float angle_x,angle_y,angle_z;
    float length = sqrt(axis.x() * axis.x() + axis.y()*axis.y()+axis.z()*axis.z());
    angle_x = std::acos(axis.x()/length);
    angle_y = std::acos(axis.y()/length);
    angle_z = std::acos(axis.z()/length);
    Eigen::Matrix4f m1,m2,m3  = Eigen::Matrix4f::Identity();
    m1<<1,0,0,0,0,cos(angle_x),-sin(angle_x),0,0,sin(angle_x),cos(angle_x),0,0,0,0,1;
    m2<<cos(angle_y),0,sin(angle_y),0,0,1,0,0,-sin(angle_y),0,cos(angle_y),0,0,0,0,1;
    m3<<cos(angle_z),-sin(angle_z),0,0,sin(angle_z),cos(angle_z),0,0,0,0,1,0,0,0,0,1;

    Eigen::Matrix4f rotation = Eigen::Matrix4f::Identity();
    rotation =m3*m2*m1*Eigen::Matrix4f::Identity();
    return rotation;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
                                      float zNear, float zFar)
{
    // Students will implement this function

    zNear = -abs(zNear);
    zFar = -abs(zFar);

    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    float t = tan(eye_fov * MY_PI / (2 * 180 ) ) * abs(zNear);

    auto r = aspect_ratio * t;

    Eigen::Matrix4f Persp2Ortho = Eigen::Matrix4f::Identity();

    Persp2Ortho << zNear, 0, 0, 0,
                    0, zNear, 0, 0,
                    0, 0, (zNear + zFar), -zNear * zFar,
                    0 ,0, 1, 0;
    Eigen::Matrix4f Ortho = Eigen::Matrix4f ::Identity();

    Eigen::Matrix4f Scale = Eigen::Matrix4f::Identity();

    Scale << 1.0/r, 0, 0, 0,
             0, 1.0/t, 0, 0,
             0, 0, 2/(zNear - zFar), 0,
             0, 0, 0, 1;
    Eigen::Matrix4f  Translation = Eigen::Matrix4f::Identity();

    Translation << 1, 0, 0, 0,
                 0, 1, 0, 0,
                 0, 0, 1, -(zNear+zFar)/2,
                 0, 0, 0, 1;
    Ortho = Scale * Translation;

    projection = Ortho * Persp2Ortho;

    return projection;
}

int main(int argc, const char** argv)
{
    float angle = 0;
    bool command_line = false;
    std::string filename = "output.png";

    if (argc >= 3) {
        command_line = true;
        angle = std::stof(argv[2]); // -r by default
        if (argc == 4) {
            filename = std::string(argv[3]);
        }
        else
            return 0;
    }

    rst::rasterizer r(700, 700);

    Eigen::Vector3f eye_pos = {0, 0, 5};

    Eigen::Vector3f rotation_axis = {1.0/sqrt(3) ,1.0/sqrt(3), 1.0/sqrt(3)};

    std::vector<Eigen::Vector3f> pos{{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}};

    std::vector<Eigen::Vector3i> ind{{0, 1, 2}};

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);

    int key = 0;
    int frame_count = 0;

    if (command_line) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(rotation_axis, angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);

        cv::imwrite(filename, image);

        return 0;
    }

    while (key != 27) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::imshow("image", image);
        key = cv::waitKey(10);

        std::cout << "frame count: " << frame_count++ << '\n';

        if (key == 'a') {
            angle += 10;
        }
        else if (key == 'd') {
            angle -= 10;
        }
    }

    return 0;
}
