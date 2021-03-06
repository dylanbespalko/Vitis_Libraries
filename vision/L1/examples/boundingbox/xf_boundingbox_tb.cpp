/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common/xf_headers.hpp"
#include "xf_boundingbox_config.h"
/*#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <opencv/cv.h>
#include <opencv/cxcore.h>
# include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>*/
#include "ap_int.h"
using namespace std;

int main(int argc, char** argv) {
    cv::Mat in_img, in_img1, out_img, diff;

    // struct timespec start_time;
    // struct timespec end_time;

    if (argc != 3) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> <number of boxes> \n");
        return -1;
    }

#if GRAY
    /*  reading in the gray image  */
    in_img = cv::imread(argv[1], 0);
    in_img1 = in_img.clone();
    int num_box = atoi(argv[2]);
#else
    /*  reading in the color image  */
    in_img = cv::imread(argv[1], 1);
    int num_box = atoi(argv[2]);
    cvtColor(in_img, in_img, cv::COLOR_BGR2RGBA);
    in_img1 = in_img.clone();
#endif

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image at %s\n", argv[1]);
        return 0;
    }

    unsigned int x_loc[MAX_BOXES], y_loc[MAX_BOXES], ROI_height[MAX_BOXES], ROI_width[MAX_BOXES];

    /////////////////////////////////////Feeding ROI/////////////////////////////////////////

    x_loc[0] = 0; // only 3-ROI are feeded, should be modified according to NUM_BOX
    y_loc[0] = 0;
    ROI_height[0] = 70;
    ROI_width[0] = 70;

    x_loc[1] = 0;
    y_loc[1] = 0;
    ROI_height[1] = 50;
    ROI_width[1] = 50;

    x_loc[2] = 50;
    y_loc[2] = 50;
    ROI_height[2] = 20;
    ROI_width[2] = 20;

    x_loc[3] = 1;
    y_loc[3] = 1;
    ROI_height[3] = 100;
    ROI_width[3] = 100;

    x_loc[4] = 0;
    y_loc[4] = 0;
    ROI_height[4] = 100;
    ROI_width[4] = 100;

//	int num_box= 3;
//////////////////////////////////end of Feeding ROI///////////////////////////////////////
#if GRAY
    int color_info[MAX_BOXES][3] = {{255}, {255}, {255}, {150}, {56}};
#else
    int color_info[MAX_BOXES][4] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {123, 234, 108, 255},
        {122, 255, 167, 255}}; // Feeding color information for each boundary should be modified if MAX_BOXES varies
#endif

#if GRAY
    out_img.create(in_img.rows, in_img.cols, in_img.depth());
    diff.create(in_img.rows, in_img.cols, in_img.depth());

#else
    diff.create(in_img.rows, in_img.cols, CV_8UC4);
    out_img.create(in_img.rows, in_img.cols, CV_8UC4);
#endif

////////////////  reference code  ////////////////
// clock_gettime(CLOCK_MONOTONIC, &start_time);

#if GRAY
    for (int i = 0; i < num_box; i++) {
        for (int c = 0; c < XF_CHANNELS(TYPE, NPIX); c++) {
            cv::rectangle(in_img1, cv::Rect(x_loc[i], y_loc[i], ROI_width[i], ROI_height[i]),
                          cv::Scalar(color_info[i][0], 0, 0), 1); // BGR format
        }
    }
#else
    for (int i = 0; i < num_box; i++) {
        for (int c = 0; c < XF_CHANNELS(TYPE, NPIX); c++) {
            cv::rectangle(in_img1, cv::Rect(x_loc[i], y_loc[i], ROI_width[i], ROI_height[i]),
                          cv::Scalar(color_info[i][0], color_info[i][1], color_info[i][2], 255), 1); // BGR format
        }
    }
#endif

    // clock_gettime(CLOCK_MONOTONIC, &end_time);
    ////float diff_latency = (end_time.tv_nsec - start_time.tv_nsec)/1e9 + end_time.tv_sec - start_time.tv_sec;
    // printf("\latency: %f ", diff_latency);

    cv::imwrite("ocv_ref.jpg", in_img1); // reference image

    //////////////////  end opencv reference code//////////

    ////////////////////// HLS TOP function call ////////////////////////////

    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPIX> imgInput(in_img.rows, in_img.cols);
    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPIX> imgOutput(in_img.rows, in_img.cols);

    xf::cv::Rect_<int>* roi = (xf::cv::Rect_<int>*)malloc(MAX_BOXES * sizeof(xf::cv::Rect_<int>));
    xf::cv::Scalar<4, unsigned char>* color =
        (xf::cv::Scalar<4, unsigned char>*)malloc(MAX_BOXES * sizeof(xf::cv::Scalar<4, unsigned char>));

    for (int i = 0, j = 0; i < (num_box); j++, i++) {
        roi[i].x = x_loc[i];
        roi[i].y = y_loc[i];
        roi[i].height = ROI_height[i];
        roi[i].width = ROI_width[i];
    }

    for (int i = 0; i < (num_box); i++) {
        for (int j = 0; j < XF_CHANNELS(TYPE, NPIX); j++) {
            color[i].val[j] = color_info[i][j];
        }
    }

    imgInput.copyTo(in_img.data);

    boundingbox_accel(imgInput, roi, color, num_box);

    out_img.data = imgInput.copyFrom();
    cv::imwrite("hls_out.jpg", out_img);

    cv::absdiff(out_img, in_img1, diff);
    cv::imwrite("diff.jpg", diff); // Save the difference image for debugging purpose

    //	 Find minimum and maximum differences.

    double minval = 256, maxval1 = 0;
    int cnt = 0;
    for (int i = 0; i < in_img1.rows; i++) {
        for (int j = 0; j < in_img1.cols; j++) {
            uchar v = diff.at<uchar>(i, j);
            if (v > 1) cnt++;
            if (minval > v) minval = v;
            if (maxval1 < v) maxval1 = v;
        }
    }
    float err_per = 100.0 * (float)cnt / (in_img1.rows * in_img1.cols);
    fprintf(stderr,
            "Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error "
            "threshold = %f\n",
            minval, maxval1, err_per);

    if (err_per > 0.0f) {
        return 1;
    }

    return 0;
}
