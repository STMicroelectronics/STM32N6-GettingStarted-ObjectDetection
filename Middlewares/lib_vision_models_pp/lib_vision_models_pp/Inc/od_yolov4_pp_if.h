/*---------------------------------------------------------------------------------------------
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file in
  * the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *--------------------------------------------------------------------------------------------*/

#ifndef __OD_YOLOV4_PP_IF_H__
#define __OD_YOLOV4_PP_IF_H__


#ifdef __cplusplus
 extern "C" {
#endif

#include "od_pp_output_if.h"


/* I/O structures for YoloV4 detector type */
/* --------------------------------------- */
typedef struct yolov4_pp_in_centroid
{
  void *pRaw_boxes;
  void *pRaw_probas;
} yolov4_pp_in_centroid_t;


typedef struct yolov4_pp_static_param {
  int32_t  nb_classes;
  int32_t  nb_total_boxes;
  int32_t  max_boxes_limit;
  float32_t conf_threshold;
  float32_t iou_threshold;
  float32_t raw_output_scale;
  int8_t raw_output_zero_point;
  int32_t nb_detect;
  void *pScratchBuff;
} yolov4_pp_static_param_t;

typedef struct
{
	int8_t  x_center;
	int8_t  y_center;
	int8_t  width;
	int8_t  height;
	int8_t  conf;
	uint8_t class_index;
} od_pp_outBuffer_s8_t;


/* Exported functions ------------------------------------------------------- */

/*!
 * @brief Resets object detection YoloV4 post processing
 *
 * @param [IN] Input static parameters
 * @retval Error code
 */
int32_t od_yolov4_pp_reset(yolov4_pp_static_param_t *pInput_static_param);


/*!
 * @brief Object detector post processing : includes output detector remapping,
 *        nms and score filtering for YoloV4.
 *
 * @param [IN] Pointer on input data
 *             Pointer on output data
 *             pointer on static parameters
 * @retval Error code
 */
int32_t od_yolov4_pp_process(yolov4_pp_in_centroid_t *pInput,
                             od_pp_out_t *pOutput,
                             yolov4_pp_static_param_t *pInput_static_param);


/*!
 * @brief Object detector post processing : includes output detector remapping,
 *        nms and score filtering for YoloV4 with 8-bits quantized inputs.
 *
 * @param [IN] Pointer on input data
 *             Pointer on output data
 *             pointer on static parameters
 * @retval Error code
 */
int32_t od_yolov4_pp_process_int8(yolov4_pp_in_centroid_t *pInput,
                                  od_pp_out_t *pOutput,
                                  yolov4_pp_static_param_t *pInput_static_param);

#ifdef __cplusplus
  }
#endif

#endif      /* __OD_YOLOV4_PP_IF_H__  */


