//
// Created by Paul KLEIN on 25/08/2016.
//


#include "GaussianRegression.h"

using namespace cv;

GaussianRegressor::GaussianRegressor(EM model)
{
  vector<Mat> covs  = model.get<vector<Mat>>("covs");
  Mat means  = model.get<Mat>("means");
  Mat wgt(model.get<Mat>("weights"));


  n_component = (unsigned int) wgt.cols;
  double sum = 0;
  for (int i = 0; i < wgt.cols; i++)
  {
    sum += wgt.at<double>(i);
    weights.push_back(wgt.at<double>(i));
  }

  for (int i = 0; i < wgt.cols; i++)
  {
    weights.push_back(wgt.at<double>(i) / sum);
  }

  meansX = buildMeansX(means);
  meansY = buildMeansY(means);


  for (unsigned int i = 0; i < covs.size(); i++)
  {
    Mat sixx(covs.at(i), Rect(Point(0, 0), Size(24, 24)));
    assert(sixx.rows == sixx.cols);
    covsXX.push_back(sixx);
    Mat sixy(covs.at(i), Rect(Point(24, 0), Size(3, 24)));
    covsXY.push_back(sixy);
    Mat siyx(covs.at(i), Rect(Point(0, 24), Size(24, 3)));
    covsYX.push_back(siyx);
    Mat siyy(covs.at(i), Rect(Point(24, 24), Size(3, 3)));
    assert(siyy.rows == siyy.cols);
    covsYY.push_back(siyy);
  }

}

std::vector<Mat> GaussianRegressor::buildMeansX(Mat means)
{
  std::vector<Mat> ux;
  for (int i = 0; i < means.rows; i++)
  {
    Mat uix(means, Rect(0, i, 24, 1));
    ux.push_back(uix);
  }
  return ux;
}

std::vector<Mat> GaussianRegressor::buildMeansY(Mat means)
{
  std::vector<Mat> uy;
  for (int i = 0; i < means.rows; i++)
  {
    Mat uiy(means, Rect(24, i, 3, 1));
    uy.push_back(uiy);
  }
  return uy;
}

Vec3b GaussianRegressor::estimate(Mat sample)
{
  std::vector<double> betas = computeBetas(sample);
  Vec3b estimate(0, 0, 0);

  for (unsigned int i = 0; i < betas.size(); i++)
  {
    Vec3b Muiysx = computeMuysx(sample, meansX.at(i), meansY.at(i), covsYX.at(i), covsXX.at(i));
    //std::cout << "BETA " << betas.at(i) << std::endl;
    //std::cout << "MU " << Muiysx << std::endl;
    double beta = 1 / betas.size();
    estimate[0] += betas.at(i) * Muiysx[0];
    estimate[1] += betas.at(i) * Muiysx[1];
    estimate[2] += betas.at(i) * Muiysx[2];
  }


  //std::cout << "ESTIMATE " << estimate << std::endl;
  //std::cout << "SAMPLE " << sample << std::endl;

  return estimate;
}

double GaussianRegressor::computeProbPdf(Mat samples, Mat cov, Mat mean)
{
  int dim = cov.rows;
  double det = determinant(cov);
  double scale = 1.0 / (pow(pow(2 * M_PI, dim) * det, 0.5));
  Mat invcov = cov.inv();
  //std::cout << "MEAN " << mean.rows << " " << mean.cols << std::endl;
  //std::cout << "samples " << samples.rows << " " << samples.cols << std::endl;
  Mat tmp1 = samples.t() - mean;
  //std::cout << "INV " << invcov.rows << " " << invcov.cols << std::endl;
  //std::cout << "TMP1 " << tmp1.rows << " " << tmp1.cols << std::endl;
  Mat tmp2 = tmp1 * invcov * tmp1.t();
  //std::cout << "TMP2 " << tmp2 << std::endl;
  //std::cout << "PROB " << scale * exp((- 1 / 2 )* tmp2.at<double>(0, 0)) << std::endl;
  return scale * exp((- 1 / 2 ) * tmp2.at<double>(0, 0));
}

Vec3b GaussianRegressor::computeMuysx(Mat sample, Mat meanX, Mat meanY, Mat covYX, Mat covXX)
{
  Mat tmp1 = meanX.t() - sample;
  Mat tmp2 = covYX * covXX.inv();
  Mat tmp3 = tmp2 * tmp1;
  Mat Muysx = meanY - tmp3.t();

  Vec3b ret(Muysx.at<uchar>(0), Muysx.at<uchar>(1), Muysx.at<uchar>(2));

  return ret;
}

std::vector<double> GaussianRegressor::computeBetas(Mat sample)
{
  double denom = 0;
  std::vector<double> betas;

  for (unsigned int i = 0; i < n_component; i++)
  {
    denom += weights[i] * computeProbPdf(sample, covsXX.at(i), meansX.at(i));
  }

  for (unsigned int i = 0; i < n_component; i++)
  {
    double beta = weights[i] * computeProbPdf(sample, covsXX.at(i), meansX.at(i));
    betas.push_back(beta / denom);
  }

  return betas;
}

