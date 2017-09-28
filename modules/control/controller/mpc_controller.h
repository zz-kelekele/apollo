/******************************************************************************
 * Copyright 2017 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

/**
 * @file
 * @brief Defines the MPCController class.
 */

#ifndef MODULES_CONTROL_CONTROLLER_MPC_CONTROLLER_H_
#define MODULES_CONTROL_CONTROLLER_MPC_CONTROLLER_H_

#include <fstream>
#include <memory>
#include <string>

#include "Eigen/Core"

#include "modules/common/configs/proto/vehicle_config.pb.h"
#include "modules/control/common/interpolation_1d.h"
#include "modules/control/common/trajectory_analyzer.h"
#include "modules/control/controller/controller.h"
#include "modules/control/filters/digital_filter.h"
#include "modules/control/filters/digital_filter_coefficients.h"
#include "modules/control/filters/mean_filter.h"

/**
 * @namespace apollo::control
 * @brief apollo::control
 */
namespace apollo {
namespace control {

/**
 * @class MPCController
 *
 * @brief LQR-Based lateral controller, to compute steering target.
 * For more details, please refer to "Vehicle dynamics and control."
 * Rajamani, Rajesh. Springer Science & Business Media, 2011.
 */
class MPCController : public Controller {
 public:
  /**
   * @brief constructor
   */
  MPCController();

  /**
   * @brief destructor
   */
  virtual ~MPCController();

  /**
   * @brief initialize MPCeral Controller
   * @param control_conf control configurations
   * @return Status initialization status
   */
  common::Status Init(const ControlConf *control_conf) override;

  /**
   * @brief compute steering target based on current vehicle status
   *        and target trajectory
   * @param localization vehicle location
   * @param chassis vehicle status e.g., speed, acceleration
   * @param trajectory trajectory generated by planning
   * @param cmd control command
   * @return Status computation status
   */
  common::Status ComputeControlCommand(
      const localization::LocalizationEstimate *localization,
      const canbus::Chassis *chassis, const planning::ADCTrajectory *trajectory,
      ControlCommand *cmd) override;

  /**
   * @brief reset MPCeral Controller
   * @return Status reset status
   */
  common::Status Reset() override;

  /**
   * @brief stop MPCeral controller
   */
  void Stop() override;

  /**
   * @brief MPCeral controller name
   * @return string controller name in string
   */
  std::string Name() const override;

 protected:
  void UpdateState(SimpleMPCDebug *debug);

  void UpdateStateAnalyticalMatching(SimpleMPCDebug *debug);

  void UpdateMatrix();

  void UpdateMatrixCompound();

  double ComputeFeedForward(double ref_curvature) const;

  double GetLateralError(
      const common::math::Vec2d &point,
      apollo::common::TrajectoryPoint *trajectory_point) const;

  void ComputeLateralErrors(const double x, const double y, const double theta,
                            const double linear_v, const double angular_v,
                            const TrajectoryAnalyzer &trajectory_analyzer,
                            SimpleMPCDebug *debug) const;
  bool LoadControlConf(const ControlConf *control_conf);
  void InitializeFilters(const ControlConf *control_conf);
  void LogInitParameters();
  void ProcessLogs(const SimpleMPCDebug *debug,
                   const canbus::Chassis *chassis);

  void CloseLogFile();

  // vehicle parameter
  common::VehicleParam vehicle_param_;

  // a proxy to analyze the planning trajectory
  TrajectoryAnalyzer trajectory_analyzer_;

  // the following parameters are vehicle physics related.
  // control time interval
  double ts_ = 0.0;
  // corner stiffness; front
  double cf_ = 0.0;
  // corner stiffness; rear
  double cr_ = 0.0;
  // distance between front and rear wheel center
  double wheelbase_ = 0.0;
  // mass of the vehicle
  double mass_ = 0.0;
  // distance from front wheel center to COM
  double lf_ = 0.0;
  // distance from rear wheel center to COM
  double lr_ = 0.0;
  // rotational inertia
  double iz_ = 0.0;
  // the ratio between the turn of the steering wheel and the turn of the wheels
  double steer_transmission_ratio_ = 0.0;
  // the maximum turn of steer
  double steer_single_direction_max_degree_ = 0.0;

  // limit steering to maximum theoretical lateral acceleration
  double max_lat_acc_ = 0.0;

  // number of control cycles look ahead (preview controller)
  int preview_window_ = 0;
  // number of states without previews, includes
  // lateral error, lateral error rate, heading error, heading error rate, station error,
  // velocity error,
  const int basic_state_size_ = 6;
  // vehicle state matrix
  Eigen::MatrixXd matrix_a_;
  // vehicle state matrix (discrete-time)
  Eigen::MatrixXd matrix_ad_;

  // control matrix
  Eigen::MatrixXd matrix_b_;
  // control matrix (discrete-time)
  Eigen::MatrixXd matrix_bd_;

  // gain matrix
  Eigen::MatrixXd matrix_k_;
  // control authority weighting matrix
  Eigen::MatrixXd matrix_r_;
  // state weighting matrix
  Eigen::MatrixXd matrix_q_;
  // updated state weighting matrix
  Eigen::MatrixXd matrix_q_updated_;
  // vehicle state matrix coefficients
  Eigen::MatrixXd matrix_a_coeff_;
  // 4 by 1 matrix; state matrix
  Eigen::MatrixXd matrix_state_;

  // heading error
  // double heading_error_ = 0.0;
  // lateral distance to reference trajectory
  // double lateral_error_ = 0.0;

  // reference heading
  // double ref_heading_ = 0.0;
  // reference trajectory curvature
  // double ref_curvature_ = 0.0;

  // heading error of last control cycle
  double previous_heading_error_ = 0.0;
  // lateral distance to reference trajectory of last control cycle
  double previous_lateral_error_ = 0.0;

  // heading error change rate over time, i.e. d(heading_error) / dt
  // double heading_error_rate_ = 0.0;
  // lateral error change rate over time, i.e. d(lateral_error) / dt
  // double lateral_error_rate_ = 0.0;

  // parameters for lqr solver; number of iterations
  int lqr_max_iteration_ = 0;
  // parameters for lqr solver; threshold for computation
  double lqr_eps_ = 0.0;

  DigitalFilter digital_filter_;

  std::unique_ptr<Interpolation1D> lat_err_interpolation_;

  std::unique_ptr<Interpolation1D> heading_err_interpolation_;

  // MeanFilter heading_rate_filter_;
  MeanFilter lateral_error_filter_;

  // for logging purpose
  std::ofstream steer_log_file_;

  const std::string name_;
};

}  // namespace control
}  // namespace apollo

#endif  // MODULES_CONTROL_CONTROLLER_MPC_CONTROLLER_H_
