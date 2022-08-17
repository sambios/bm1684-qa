//
// Created by yuan on 11/8/21.
//
#include "opencv2/opencv.hpp"
struct FaceRect {
    float x1, y1;
    float x2, y2;
};
struct FaceLandmark {
    float x[5];
    float y[5];
    float confidence;
};

using FaceLandmarkPoints=std::vector<FaceLandmark>;

static cv::Mat tformfwd(const cv::Mat &trans, const cv::Mat &uv)
{
    cv::Mat uv_h = cv::Mat::ones(uv.rows, 3, CV_64FC1);
    uv.copyTo(uv_h(cv::Rect(0, 0, 2, uv.rows)));
    cv::Mat xv_h = uv_h * trans;
    return xv_h(cv::Rect(0, 0, 2, uv.rows));
}

static cv::Mat find_none_flectives_similarity(const cv::Mat &uv, const cv::Mat &xy)
{
    cv::Mat A = cv::Mat::zeros(2 * xy.rows, 4, CV_64FC1);
    cv::Mat b = cv::Mat::zeros(2 * xy.rows, 1, CV_64FC1);
    cv::Mat x = cv::Mat::zeros(4, 1, CV_64FC1);

    xy(cv::Rect(0, 0, 1, xy.rows)).copyTo(A(cv::Rect(0, 0, 1, xy.rows))); // x
    xy(cv::Rect(1, 0, 1, xy.rows)).copyTo(A(cv::Rect(1, 0, 1, xy.rows))); // y
    A(cv::Rect(2, 0, 1, xy.rows)).setTo(1.);

    xy(cv::Rect(1, 0, 1, xy.rows))
            .copyTo(A(cv::Rect(0, xy.rows, 1, xy.rows))); // y
    xy(cv::Rect(0, 0, 1, xy.rows))
            .copyTo(A(cv::Rect(1, xy.rows, 1, xy.rows))); //-x
    A(cv::Rect(1, xy.rows, 1, xy.rows)) *= -1;
    A(cv::Rect(3, xy.rows, 1, xy.rows)).setTo(1.);

    uv(cv::Rect(0, 0, 1, uv.rows)).copyTo(b(cv::Rect(0, 0, 1, uv.rows)));
    uv(cv::Rect(1, 0, 1, uv.rows)).copyTo(b(cv::Rect(0, uv.rows, 1, uv.rows)));

    cv::solve(A, b, x, cv::DECOMP_SVD);
    cv::Mat trans_inv = (cv::Mat_<double>(3, 3) << x.at<double>(0),
            -x.at<double>(1),
            0,
            x.at<double>(1),
            x.at<double>(0),
            0,
            x.at<double>(2),
            x.at<double>(3),
            1);
    cv::Mat trans = trans_inv.inv(cv::DECOMP_SVD);
    trans.at<double>(0, 2) = 0;
    trans.at<double>(1, 2) = 0;
    trans.at<double>(2, 2) = 1;
    return trans;
}

static cv::Mat find_similarity(const cv::Mat &uv, const cv::Mat &xy)
{
    cv::Mat trans1 = find_none_flectives_similarity(uv, xy);
    cv::Mat xy_reflect = xy;
    xy_reflect(cv::Rect(0, 0, 1, xy.rows)) *= -1;
    cv::Mat trans2r = find_none_flectives_similarity(uv, xy_reflect);
    cv::Mat reflect = (cv::Mat_<double>(3, 3) << -1, 0, 0, 0, 1, 0, 0, 0, 1);

    cv::Mat trans2 = trans2r * reflect;
    cv::Mat xy1 = tformfwd(trans1, uv);
    double norm1 = cv::norm(xy1 - xy);

    cv::Mat xy2 = tformfwd(trans2, uv);
    double norm2 = cv::norm(xy2 - xy);

    cv::Mat trans;
    if (norm1 < norm2)
    {
        trans = trans1;
    }
    else
    {
        trans = trans2;
    }
    return trans;
}

static cv::Mat get_similarity_transform(const std::vector<cv::Point2f> &src_points,
                                        const std::vector<cv::Point2f> &dst_points,
                                        bool reflective = true)
{
    cv::Mat trans;
    cv::Mat src(
            (int)src_points.size(), 2, CV_32FC1, (void *)(&src_points[0].x));
    src.convertTo(src, CV_64FC1);

    cv::Mat dst(
            (int)dst_points.size(), 2, CV_32FC1, (void *)(&dst_points[0].x));
    dst.convertTo(dst, CV_64FC1);

    if (reflective)
    {
        trans = find_similarity(src, dst);
    }
    else
    {
        trans = find_none_flectives_similarity(src, dst);
    }
    cv::Mat trans_cv2 = trans(cv::Rect(0, 0, 2, trans.rows)).t();
    return trans_cv2;
}

static cv::Mat calc_transform_matrix(const FaceRect rect,
                                     const FaceLandmark facePt,
                                     int width,
                                     int height)
{
    const int ReferenceWidth = 96;
    const int ReferenceHeight = 112;
    std::vector<cv::Point2f> detect_points;
    for (int j = 0; j < 5; ++j)
    {
        cv::Point2f e;
        e.x = facePt.x[j] + rect.x1;
        e.y = facePt.y[j] + rect.y1;
        detect_points.push_back(e);
    }
    std::vector<cv::Point2f> reference_points;
    reference_points.push_back(cv::Point2f(30.29459953, 51.69630051));
    reference_points.push_back(cv::Point2f(65.53179932, 51.50139999));
    reference_points.push_back(cv::Point2f(48.02519989, 71.73660278));
    reference_points.push_back(cv::Point2f(33.54930115, 92.36550140));
    reference_points.push_back(cv::Point2f(62.72990036, 92.20410156));
    for (int j = 0; j < 5; ++j)
    {
        reference_points[j].x += (width - ReferenceWidth) / 2.0f;
        reference_points[j].y += (height - ReferenceHeight) / 2.0f;
    }
    cv::Mat tfm = get_similarity_transform(detect_points, reference_points);

    // inverse matrix.
    cv::Mat tM;
    tfm.convertTo(tM, CV_32F);
    invertAffineTransform(tM.clone(), tM);
    CV_Assert(tM.total() == 6);

    return tM;
}

int main(int argc, char *argv[])
{
    cv::Mat img0 = cv::imread("/home/yuan/bitmain/face_align/face_0.jpg");
    cv::Mat img1 ;
    FaceRect rect;
    int width = 0;
    int height = 0;
    FaceLandmark landmark;

    cv::Mat matrix = calc_transform_matrix(rect, landmark, width, height);
    cv::warpAffine(img0, img1, , cv::Size(96,112));
    cv::imwrite("affine0.jpg", img1);
}