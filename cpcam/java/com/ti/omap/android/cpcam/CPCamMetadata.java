/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.ti.omap.android.cpcam;

import android.graphics.*;
import java.nio.ByteBuffer;

/**
 * Used for passing camera related metadata
 */
public class CPCamMetadata {

    protected CPCamMetadata() {
    }

    public static CPCamMetadata getMetadata(CPCamBufferQueue st) {
        return nativeRetrieveMetadata(st, 0);
    }

    public static CPCamMetadata getMetadata(CPCamBufferQueue st, int slot) {
        return nativeRetrieveMetadata(st, slot);
    }

    public static class BSCPosition {

        /**
         * The number of row/column sums cannot exceed 1920, implies:
         *  -  (vectors + 1) * (vertical_number) <=1920, for row sums
         *  -  (vectors + 1) * (horizontal_number) <=1920, for column sums
         */
        BSCPosition () {
        }

        /**
         * vectors : number of row/column sum vectors. Max value = 4
         */
        public int vectors;

        /**
         * shift : down-shift of input data
         */
        public int shift;

        /**
         * vertical_position : vertical position of first pixel to be summed
         */
        public int verticalPosition;

        /**
         * horizontal_position : horizontal position of first pixel to be summed
         */
        public int horizontalPosition;

        /**
         * vertical_number : number of pixels sampled vertically
         */
        public int verticalNumber;

        /**
         * horizontal_number : number of pixels sampled horizontally
         */
        public int horizontalNumber;

        /**
         * vertical_skip : vertical spacing between adjacent pixels to be summed
         */
        public int verticalSkip;

        /**
         * horizontal_skip : horizontal pixel spacing between adjacent pixels to be summed
         */
        public int horizontalSkip;
    }

    public static class H3AConfig {

        H3AConfig () {
        }

        /**
         * vertical_position: vertical start point of paxel grid
         * w.r.t first pixel of input image frame
         */
        public int verticalPosition;

        /**
         * vertical_size: vertical paxel size
         */
        public int verticalSize;

        /**
         * horizontal_position: horizontal start point of paxel grid
         * w.r.t first pixel of input image frame
         */
        public int horizontalPosition;

        /**
         * horizontal_size: horizontal paxel size
         */
        public int horizontalSize;

        /**
         * vertical_count: num of vert paxels. AF/AEWB paxels
         * are always adjacent to each other
         */
        public int verticalCount;

        /**
         * vetical_increment: num of pixels to skip within a paxel, vertically
         */
        public int veticalIncrement;

        /**
         * horizontal_count: num of horz paxels.
         * AF/AEWB paxels are always adjacent to each other
         */
        public int horizontalCount;

        /**
         * horizontal_increment: num of pixels to skip within a paxel,
         * horizontally
         */
        public int horizontalIncrement;
    }

    /**
     * Used to store the information about frame
     * number in processing sequence (i.e preview)
     */
    public int frameNumber;

    /**
     * Used to store the information about shot number
     * in a burst sequence.
     */
    public int shotNumber;

    /**
     * Used to store analog gain information for
     * current frame. Metadata is represented as 100*EV.
     */
    public int analogGain;

    /**
     * Used for storing analog gain information
     * requested by application for current frame. Metadata is represented as 100*EV.
     */
    public int analogGainReq;

    /**
     * Used for storing the analog gain
     * lower limit for current frame. Metadata is represented as 100*EV.
     */
    public int analogGainMin;

    /**
     * Used for storing the analog gain
     * upper limit for current frame. Metadata is represented as 100*EV.
     */
    public int analogGainMax;

    /**
     * Used for storing the analog gain
     * deviation after flicker reduction for current frame. Metadata is represented as 100*EV.
     */
    public int analogGainDev;

    /**
     * Used for storing analog gain error for
     * current frame. Represents the difference between requested value and actual value.
     */
    public int analogGainError;

    /**
     * Used for storing the exposure time for current frame.
     * Metadata is represented in us.
     */
    public int exposureTime;

    /**
     * Used for storing the exposure time requested by
     * application for current frame. Metadata is represented in us.
     */
    public int exposureTimeReq;

    /**
     * Used for storing the exposure time lower limit for
     * current frame. Metadata is represented in us.
     */
    public int exposureTimeMin;

    /**
     * Used for storing the exposure time upper limit for
     * current frame. Metadata is represented in us.
     */
    public int exposureTimeMax;

    /**
     * Used for storing the exposure time
     * deviation after flicker reduction for current frame. Metadata is represented in us.
     */
    public int exposureTimeDev;

    /**
     * Used for storing the time difference between
     * requested exposure time and actual exposure time.
     */
    public int exposureTimeError;

    /**
     * Used for storing the current total exposure
     * compensation requested by application for current frame.  Metadata is represented as 100*EV.
     */
    public int exposureCompensationReq;

    /**
     * Used for storing current total exposure
     * deviation for current frame.  Metadata is represented as 100*EV.
     */
    public int exposureDev;

    /**
     * Represents the timestamp in terms of a reference clock.
     */
    public long timestamp;

    /**
     * Represents the temperature of current scene in Kelvin
     */
    public int awbTemp;

    /**
     * Represent gains applied to each RGGB color channel.
     */
    public int gainR;
    public int gainGR;
    public int gainGB;
    public int gainB;

    /**
     * Represent offsets applied to each RGGB color channel.
     */
    public int offsetR;
    public int offsetGR;
    public int offsetGB;
    public int offsetB;

    /**
     * Used to store the current
     * lens shading correction table.  The table consists of an
     * N by M array of elements.  Each element has 4 integer values
     * ranging from 0 to 1000, corresponding to a multiplier for
     * each of the Bayer color filter channels (R, Gr, Gb, B).
     * Correction is performed on pixels in a Bayer image by interpolating
     * the corresponding color filter channel in the array, and then
     * multiplying by (value/1000).
     */
    public ByteBuffer lscTable;

    /**
     * Indicates whether LSC table is applied or not
     */
    public int lscTableApplied;

    /**
     * An array of the detected faces. The length is numberOfFaces.
     * The Face rectangles have to following layout:
     * int top - Top coordinate of the face rectangle,
     * int left - Left coordinate of the face rectangle,
     * int bottom - Bottom coordinate of the face rectangle.
     * int right - Right coordnate of the face rectangle.
     */
    public ByteBuffer faces;

    public int numberOfFaces;

    /**
     * Width of the auxiliary image
     */
    public int auxImageWidth;

    /**
     * Height of the auxiliary image
     */
    public int auxImageHeight;

    /**
     * Auxiliary image buffer NV12 pixelformat
     */
    public ByteBuffer auxImage;

    /**
     * Element to be summed
     *   Y = 0,
     *   Cb = 1,
     *   Cr = 2,
    */
    public int bscColorElement;

    /**
     *  BSC row sum descriptor
     */
    BSCPosition bscRowPosition;

    /**
     * BSC column sum descriptor
     */
    BSCPosition bscColPosition;

    /**
     * Each value corresponds to sum value in a row.
     * Num of row sums = row_position.vectors * row_position.vertical_number
     */
    public ByteBuffer bscRowSum;

    /**
     * Each value corresponds to sum value in a row.
     * Num of row sums = row_position.vectors * row_position.vertical_number
     */
    public ByteBuffer bscColSum;

    /**
     * When Vertical focus is disabled, R,G,B location w.r.t.
     * to paxel start location is specified by this field.
     *      AF_RGBPOSITION_BAYER_GR_GB = 0
     *      AF_RGBPOSITION_BAYER_RG_GB = 1
     *      AF_RGBPOSITION_BAYER_GR_BG = 2
     *      AF_RGBPOSITION_BAYER_RG_BG = 3
     *      AF_RGBPOSITION_CUSTOM_GG_RB = 4
     *      AF_RGBPOSITION_CUSTOM_RB_GG = 5
     */
    public int afBayeRGBPosition;

    /**
     * If enabled, peak for FV, FV^2 is computed for a paxel.
     * If disabled, average of FV, FV^2 is computed for a paxel.
     */
    public int afEnableAFPeakMode;

    /**
     * Whether vertical focus is enabled.
     */
    public int afEnableAFVertical;

    /**
     * AF paxel description
     */
    public H3AConfig afPaxelWindow;

    /**
     * Output AF buffer. Data is ordered in paxels:
     *
     * g_paxel - Paxel information for green color
     * rb_paxel - Paxel information for red/blue color
     * br_paxel - Paxel information for blue/red color
     *
     * Each paxel consists of :
     *     int sum - Sum of the pixels used to arrive at
     *               the statistics for a paxel
     *     int focus_value_sum - Focus Value (sum/peak)
     *                           for a paxel
     *     int focus_value_sqr_sum - Focus Value Squared
     *                               (sum/peak) for a paxel
     *     int reserved - To be ignored
     *   ------------------------------------
     *  | G paxel                            |
     *  | ------   ------   ------   ------  |
     *  ||      | |      | |      | |      | |
     *  || sum  | |f_sum | |f_sum | | rsv  | |
     *  ||      | |      | |sqr   | |      | |
     *  | ------   ------   ------   ------  |
     *   ------------------------------------
     *
     *   ------------------------------------
     *  | RB paxel                           |
     *  | ------   ------   ------   ------  |
     *  ||      | |      | |      | |      | |
     *  || sum  | |f_sum | |f_sum | | rsv  | |
     *  ||      | |      | |sqr   | |      | |
     *  | ------   ------   ------   ------  |
     *   ------------------------------------
     *
     *   ------------------------------------
     *  | BR paxel                           |
     *  | ------   ------   ------   ------  |
     *  ||      | |      | |      | |      | |
     *  || sum  | |f_sum | |f_sum | | rsv  | |
     *  ||      | |      | |sqr   | |      | |
     *  | ------   ------   ------   ------  |
     *   ------------------------------------
     */
    public ByteBuffer afPaxelStatistics;

    /**
     * AEWB mode :
     * AEWB_MODE_SUM_OF_SQUARE = 0 - Sum of square calculated
     *                               across sub-samples in a paxel.
     * AEWB_MODE_MINMAX = 1 - Min-max calculted across sub-samples
     *                        in a paxel.
     * AEWB_MODE_SUM_ONLY = 2 - Only Sum calculated across sub-samples
     *                           in a paxel.
     */
    public int aewbMode;

    /**
     * Threshold against which pixel values are compared
     */
    public int aewbThresholdPixelValue;

    /**
     * Right shift value applied on result of pixel accumulation
     */
    public int aewbAccumulationShift;

    /**
     * AE/AWB paxel description
     */
    public H3AConfig aewbPaxelWindow;

    /**
     * Output AE/AWB buffer, containing:
     * subsampled_acc_values[4] - Sub sample accumulator(s), not-clipped.
     *                            Separate for each pixel in 2x2 sub-sample.
     * saturator_acc_values[4] - Saturator accumulator(s), clipped based upon threshold.
     *                           Separate for each pixel in 2x2 sub-sample.
     * nUnsaturatedCount[2] - Count of unsaturated 2x2 sub-samples in a paxel.
     *                        (LS 16-bits stored in [0], MS stored in [1])
     */
    public ByteBuffer aewbPaxelStatistics;

    private static native CPCamMetadata nativeRetrieveMetadata(CPCamBufferQueue st, int slot);

    /*
     * We use a class initializer to allow the native code to cache some
     * field offsets.
     */
    private static native void nativeClassInit();
    static { nativeClassInit(); }
}
