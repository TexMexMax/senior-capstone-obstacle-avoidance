#include <HAL_VL53L0X.h>

#define VERSION_REQUIRED_MAJOR 1 ///< Required sensor major version
#define VERSION_REQUIRED_MINOR 0 ///< Required sensor minor version
#define VERSION_REQUIRED_BUILD 1 ///< Required sensor build

#define STR_HELPER(x) #x     ///< a string helper
#define STR(x) STR_HELPER(x) ///< string helper wrapper

/**************************************************************************/
/*!
    @brief  Setups the I2C interface and hardware
    @param  i2c_addr Optional I2C address the sensor can be found on. Default is
   0x29
    @param debug Optional debug flag. If true, debug information will print out
   via Serial.print during setup. Defaults to false.
    @param  i2c Optional I2C bus the sensor is located on. Default is Wire
    @returns True if device is set up, false on any failure
*/
/**************************************************************************/
bool HAL_VL53L0X::begin(uint8_t i2c_addr, bool debug, I2C_HandleTypeDef* hi2c) {
  int32_t status_int;
  int32_t init_done = 0;

  uint32_t refSpadCount;
  uint8_t isApertureSpads;
  uint8_t VhvSettings;
  uint8_t PhaseCal;

  // Initialize Comms
  pMyDevice->I2cDevAddr = VL53L0X_I2C_ADDR; // default
  pMyDevice->comms_type = 1;
  pMyDevice->comms_speed_khz = 400;
  pMyDevice->hi2c = hi2c;

  // unclear if this is even needed:
  if (VL53L0X_IMPLEMENTATION_VER_MAJOR != VERSION_REQUIRED_MAJOR ||
      VL53L0X_IMPLEMENTATION_VER_MINOR != VERSION_REQUIRED_MINOR ||
      VL53L0X_IMPLEMENTATION_VER_SUB != VERSION_REQUIRED_BUILD) {

    Status = VL53L0X_ERROR_NOT_SUPPORTED;

    return false;
  }

  Status = VL53L0X_DataInit(&MyDevice); // Data initialization

  if (!setAddress(i2c_addr)) {
    return false;
  }

  Status = VL53L0X_GetDeviceInfo(&MyDevice, &DeviceInfo);


  if (Status == VL53L0X_ERROR_NONE) {

    Status = VL53L0X_StaticInit(pMyDevice); // Device Initialization
  }

  if (Status == VL53L0X_ERROR_NONE) {

    Status = VL53L0X_PerformRefSpadManagement(
        pMyDevice, &refSpadCount, &isApertureSpads); // Device Initialization

  }

  if (Status == VL53L0X_ERROR_NONE) {

    Status = VL53L0X_PerformRefCalibration(pMyDevice, &VhvSettings,
                                           &PhaseCal); // Device Initialization
  }

  if (Status == VL53L0X_ERROR_NONE) {
    // no need to do this when we use VL53L0X_PerformSingleRangingMeasurement

    Status = VL53L0X_SetDeviceMode(
        pMyDevice,
        VL53L0X_DEVICEMODE_SINGLE_RANGING); // Setup in single ranging mode
  }

  // Enable/Disable Sigma and Signal check
  if (Status == VL53L0X_ERROR_NONE) {
    Status = VL53L0X_SetLimitCheckEnable(
        pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1);
  }

  if (Status == VL53L0X_ERROR_NONE) {
    Status = VL53L0X_SetLimitCheckEnable(
        pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1);
  }

  if (Status == VL53L0X_ERROR_NONE) {
    Status = VL53L0X_SetLimitCheckEnable(
        pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, 1);
  }

  if (Status == VL53L0X_ERROR_NONE) {
    Status = VL53L0X_SetLimitCheckValue(
        pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD,
        (FixPoint1616_t)(1.5 * 0.023 * 65536));
  }

  if (Status == VL53L0X_ERROR_NONE) {
    return true;
  } else {

    return false;
  }
}

/**************************************************************************/
/*!
    @brief  Change the I2C address of the sensor
    @param  newAddr the new address to set the sensor to
    @returns True if address was set successfully, False otherwise
*/
/**************************************************************************/
bool HAL_VL53L0X::setAddress(uint8_t newAddr) {
  newAddr &= 0x7F;

  Status = VL53L0X_SetDeviceAddress(pMyDevice, newAddr * 2); // 7->8 bit

  HAL_Delay(10);

  if (Status == VL53L0X_ERROR_NONE) {
    pMyDevice->I2cDevAddr = newAddr; // 7 bit addr
    return true;
  }
  return false;
}

/**************************************************************************/
/*!
    @brief  get a ranging measurement from the device
    @param  RangingMeasurementData the pointer to the struct the data will be
   stored in
    @param debug Optional debug flag. If true debug information will print via
   Serial.print during execution. Defaults to false.
    @returns True if address was set successfully, False otherwise
*/
/**************************************************************************/
VL53L0X_Error HAL_VL53L0X::getSingleRangingMeasurement(
    VL53L0X_RangingMeasurementData_t *RangingMeasurementData, bool debug) {
  VL53L0X_Error Status = VL53L0X_ERROR_NONE;
  FixPoint1616_t LimitCheckCurrent;

  /*
   *  Step  4 : Test ranging mode
   */

  if (Status == VL53L0X_ERROR_NONE) {
    Status = VL53L0X_PerformSingleRangingMeasurement(pMyDevice,
                                                     RangingMeasurementData);

    if (debug) {
      printRangeStatus(RangingMeasurementData);
    }

    if (debug) {
      VL53L0X_GetLimitCheckCurrent(pMyDevice,
                                   VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD,
                                   &LimitCheckCurrent);

    }
  }

  return Status;
}

/**************************************************************************/
/*!
    @brief  print a ranging measurement out via Serial.print in a human-readable
   format
    @param pRangingMeasurementData a pointer to the ranging measurement data
*/
/**************************************************************************/
void HAL_VL53L0X::printRangeStatus(
    VL53L0X_RangingMeasurementData_t *pRangingMeasurementData) {
  char buf[VL53L0X_MAX_STRING_LENGTH];
  uint8_t RangeStatus;

  /*
   * New Range Status: data is valid when pRangingMeasurementData->RangeStatus =
   * 0
   */

  RangeStatus = pRangingMeasurementData->RangeStatus;

  VL53L0X_GetRangeStatusString(RangeStatus, buf);

  printf("Range Status: %d\n", RangeStatus);
}
