// Authors: Pierre-Yves Lajoie, Yun Chang

#ifndef GEOM_UTILS_TYPES_H
#define GEOM_UTILS_TYPES_H

// enables correct operations of GTSAM (correct Jacobians)
#define SLOW_BUT_CORRECT_BETWEENFACTOR

#include <gtsam/geometry/Pose3.h>
#include <gtsam/geometry/Rot3.h>
#include <gtsam/slam/BetweenFactor.h>
#include <gtsam/slam/PriorFactor.h>
#include <gtsam/linear/NoiseModel.h>

#include "RobustPGO/logger.h"

#include <map>
#include <vector>

/** \namespace graph_utils
 *  \brief This namespace encapsulates utility functions to manipulate graphs
 */
namespace RobustPGO {

/** \getting the dimensions of various Lie types
*   \simple helper functions */
template<class T>
static const size_t getRotationDim() {
  // get rotation dimension of some gtsam object
  BOOST_CONCEPT_ASSERT((gtsam::IsLieGroup<T>));
  T sample_object;
  return sample_object.rotation().dimension;
}

template<class T>
static const size_t getTranslationDim() {
  // get translation dimension of some gtsam object
  BOOST_CONCEPT_ASSERT((gtsam::IsLieGroup<T>));
  T sample_object;
  return sample_object.translation().dimension;
}

template<class T>
static const size_t getDim(){
  // get overall dimension of some gtsam object
  BOOST_CONCEPT_ASSERT((gtsam::IsLieGroup<T>));
  T sample_object;
  return sample_object.dimension;
}

/** \struct PoseWithCovariance
 *  \brief Structure to store a pose and its covariance data
 *  \currently supports gtsam::Pose2 and gtsam::Pose3
 */
template <class T>
struct PoseWithCovariance {
  /* variables ------------------------------------------------ */
  /* ---------------------------------------------------------- */
  T pose; // ex. gtsam::Pose3
  gtsam::Matrix covariance_matrix;

  /* default constructor -------------------------------------- */
  PoseWithCovariance() {
    pose =  T();
    const size_t dim = getDim<T>();
    gtsam::Matrix covar =
        Eigen::MatrixXd::Zero(dim, dim); // initialize as zero
    covariance_matrix = covar;
  }

  /* basic constructor ---------------------------------------- */
  PoseWithCovariance(T pose_in, gtsam::Matrix matrix_in) {
    pose = pose_in;
    covariance_matrix = matrix_in;
  }

  /* construct from gtsam prior factor ------------------------ */
  PoseWithCovariance(const gtsam::PriorFactor<T>& prior_factor) {
    T value = prior_factor.prior();
    const size_t dim = getDim<T>();
    gtsam::Matrix covar =
        Eigen::MatrixXd::Zero(dim, dim); // initialize as zero

    pose = value;
    covariance_matrix = covar;
  }

  /* construct from gtsam between factor  --------------------- */
  PoseWithCovariance(const gtsam::BetweenFactor<T>& between_factor) {
    pose = between_factor.measured();
    gtsam::Matrix covar = boost::dynamic_pointer_cast<gtsam::noiseModel::Gaussian>
        (between_factor.get_noiseModel())->covariance();

    // prevent propagation of nan values in the edge case
    const int dim = getDim<T>();
    const int r_dim = getRotationDim<T>();
    const int t_dim = getTranslationDim<T>();
    if (std::isnan(covar.block(0,0,r_dim,r_dim).trace())) {
      // only keep translation part
      Eigen::MatrixXd temp = Eigen::MatrixXd::Zero(dim, dim);
      temp.block(r_dim,r_dim,t_dim,t_dim) =
          covar.block(r_dim,r_dim,t_dim,t_dim);
      covar = temp;
    }

    covariance_matrix = covar;

  }

  /* method to combine two poses (along with their covariances) */
  /* ---------------------------------------------------------- */
  PoseWithCovariance compose(const PoseWithCovariance& other) const {
    PoseWithCovariance<T> out;
    gtsam::Matrix Ha, Hb;

    out.pose = pose.compose(other.pose, Ha, Hb);
    out.covariance_matrix = Ha * covariance_matrix * Ha.transpose() +
        Hb * other.covariance_matrix * Hb.transpose();

    return out;
  }

  /* method to invert a pose along with its covariance -------- */
  /* ---------------------------------------------------------- */
  PoseWithCovariance inverse() const {
    gtsam::Matrix Ha;
    PoseWithCovariance<T> out;
    out.pose = pose.inverse(Ha);
    out.covariance_matrix = Ha * covariance_matrix * Ha.transpose();

    return out;
  }

  /* method to find the transform between two poses ------------ */
  /* ----------------------------------------------------------- */
  PoseWithCovariance between(const PoseWithCovariance& other) const {

    PoseWithCovariance<T> out;
    gtsam::Matrix Ha, Hb;
    out.pose = pose.between(other.pose, Ha, Hb); // returns between in a frame

    out.covariance_matrix = other.covariance_matrix -
        Ha * covariance_matrix * Ha.transpose();
    bool pos_semi_def = true;
    // compute the Cholesky decomp
    Eigen::LLT<Eigen::MatrixXd> lltCovar1(out.covariance_matrix);
    if(lltCovar1.info() == Eigen::NumericalIssue){
      pos_semi_def = false;
    }

    if (!pos_semi_def) {
      other.pose.between(pose, Ha, Hb); // returns between in a frame
      out.covariance_matrix = covariance_matrix -
          Ha * other.covariance_matrix * Ha.transpose();

      // Check if positive semidef
      Eigen::LLT<Eigen::MatrixXd> lltCovar2(out.covariance_matrix);
      // if(lltCovar2.info() == Eigen::NumericalIssue){
      //   log<WARNING>("Warning: Covariance matrix between two poses not PSD");
      // }
    }
    return out;
  }

  double norm() const {
    // calculate mahalanobis norm
    gtsam::Vector log = T::Logmap(pose);
    return std::sqrt(log.transpose() * gtsam::inverse(covariance_matrix) * log);
  }
};

/** \struct PoseWithDistance
 *  \brief Structure to store a pose and its distance from start
 *  \currently supports gtsam::Pose2 and gtsam::Pose3
 */
template <class T>
struct PoseWithDistance {
  /* variables ------------------------------------------------ */
  /* ---------------------------------------------------------- */
  T pose; // ex. gtsam::Pose3
  double distance;

  /* default constructor -------------------------------------- */
  PoseWithDistance() {
    pose =  T();
    distance = 0;
  }

  /* basic constructor ---------------------------------------- */
  PoseWithDistance(T pose_in, double dist_in) {
    pose = pose_in;
    distance = dist_in;
  }

  /* construct from gtsam prior factor ------------------------ */
  PoseWithDistance(const gtsam::PriorFactor<T>& prior_factor) {
    T value = prior_factor.prior();
    pose = value;
    distance = 0.0;
  }

  /* construct from gtsam between factor  --------------------- */
  PoseWithDistance(const gtsam::BetweenFactor<T>& between_factor) {
    pose = between_factor.measured();
    distance = between_factor.measured.translation().norm();
  }

  /* method to combine two poses (along with their covariances) */
  /* ---------------------------------------------------------- */
  PoseWithDistance compose(const PoseWithDistance& other) const {
    PoseWithDistance<T> out;

    out.pose = pose.compose(other.pose);
    out.distance = distance + other.pose.translation().norm();
    return out;
  }

  /* method to invert a pose along with its covariance -------- */
  /* ---------------------------------------------------------- */
  PoseWithDistance inverse() const {
    PoseWithDistance<T> out;

    out.pose = pose.inverse();
    out.distance = distance;
    return out;
  }

  /* method to find the transform between two poses ------------ */
  /* ----------------------------------------------------------- */
  PoseWithDistance between(const PoseWithDistance& other) const {
    PoseWithDistance<T> out;
    out.pose = pose.between(other.pose); // returns between in a frame

    out.distance = abs(other.distance - distance);
    return out;
  }

  double norm() const {
    // calculate mahalanobis norm
    gtsam::Vector log = T::Logmap(pose);
    return std::sqrt(log.transpose() * log) / distance;
  }
};

/** \struct Transform
 *  \brief Structure defining a transformation between two poses
 */
template <class T>
struct Transform {
    gtsam::Key i, j;
    PoseWithCovariance<T> pose;
    bool is_separator;
};

/** \struct Transforms
 *  \brief Structure defining a std::map of transformations
 */
template <class T>
struct Transforms {
    gtsam::Key start_id, end_id;
    std::map<std::pair<gtsam::Key,gtsam::Key>, Transform<T>> transforms;
};

/** \struct TrajectoryPose
 *  \brief Structure defining a pose in a robot trajectory
 */
template <class T>
struct TrajectoryPose {
    gtsam::Key id;
    PoseWithCovariance<T> pose;
};

/** \struct Trajectory
 *  \brief Structure defining a robot trajectory
 */
template <class T>
struct Trajectory {
    gtsam::Key start_id, end_id;
    std::map<gtsam::Key, TrajectoryPose<T>> trajectory_poses;
};

/** \struct DistTrajectory
 *  \brief Same as Trajectory but instead of covariance tracks distance
 */
template <class T>
struct DistTrajectory {
    gtsam::Key start_id, end_id;
    std::map<gtsam::Key, PoseWithDistance<T>> trajectory_poses;
};

}

#endif