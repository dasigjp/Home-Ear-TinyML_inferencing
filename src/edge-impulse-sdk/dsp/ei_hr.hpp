/*
 * Copyright (c) 2024 EdgeImpulse Inc.
 *
 * Generated by Edge Impulse and licensed under the applicable Edge Impulse
 * Terms of Service. Community and Professional Terms of Service
<<<<<<< HEAD
 * (https://edgeimpulse.com/legal/terms-of-service) or Enterprise Terms of
 * Service (https://edgeimpulse.com/legal/enterprise-terms-of-service),
=======
 * (https://docs.edgeimpulse.com/page/terms-of-service) or Enterprise Terms of
 * Service (https://docs.edgeimpulse.com/page/enterprise-terms-of-service),
>>>>>>> ab9923a28fc5410c832133804f4acbe53395f851
 * according to your product plan subscription (the “License”).
 *
 * This software, documentation and other associated files (collectively referred
 * to as the “Software”) is a single SDK variation generated by the Edge Impulse
 * platform and requires an active paid Edge Impulse subscription to use this
 * Software for any purpose.
 *
 * You may NOT use this Software unless you have an active Edge Impulse subscription
 * that meets the eligibility requirements for the applicable License, subject to
 * your full and continued compliance with the terms and conditions of the License,
 * including without limitation any usage restrictions under the applicable License.
 *
 * If you do not have an active Edge Impulse product plan subscription, or if use
 * of this Software exceeds the usage limitations of your Edge Impulse product plan
 * subscription, you are not permitted to use this Software and must immediately
 * delete and erase all copies of this Software within your control or possession.
 * Edge Impulse reserves all rights and remedies available to enforce its rights.
 *
 * Unless required by applicable law or agreed to in writing, the Software is
 * distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing
 * permissions, disclaimers and limitations under the License.
 */
#ifndef HR_PPG_HPP
#define HR_PPG_HPP

#include "edge-impulse-sdk/dsp/numpy.hpp"
#include "edge-impulse-sdk/dsp/ei_dsp_handle.h"
#include "edge-impulse-enterprise/hr/hr_ppg.hpp"
#include "edge-impulse-enterprise/hr/hrv.hpp"
#include "edge-impulse-sdk/dsp/memory.hpp"

// Need a wrapper to get ei_malloc used
// cppyy didn't like this override for some reason
<<<<<<< HEAD
class hrv_wrap : public ei::hrv::ibi_to_hrv {
=======
class hrv_wrap : public ei::hrv::beats_to_hrv {
>>>>>>> ab9923a28fc5410c832133804f4acbe53395f851
public:
    // Boilerplate below here
    void* operator new(size_t size) {
        // Custom memory allocation logic here
        return ei_malloc(size);
    }

    void operator delete(void* ptr) {
        // Custom memory deallocation logic here
        ei_free(ptr);
    }
    // end boilerplate

    // Use the same constructor as the parent
<<<<<<< HEAD
    using ei::hrv::ibi_to_hrv::ibi_to_hrv;
=======
    using ei::hrv::beats_to_hrv::beats_to_hrv;
>>>>>>> ab9923a28fc5410c832133804f4acbe53395f851
};

class hr_class : public DspHandle {
public:
    int print() override {
        ei_printf("Last HR: %f\n", ppg._res.hr);
        return ei::EIDSP_OK;
    }

    int extract(
        ei::signal_t *signal,
        ei::matrix_t *output_matrix,
        void *config_ptr,
        const float frequency,
        ei_impulse_result_t *result) override
    {
        using namespace ei;

        // Using reference avoids a copy
        ei_dsp_config_hr_t& config = *((ei_dsp_config_hr_t*)config_ptr);
        size_t floats_per_inc = ppg.win_inc_samples * ppg.axes;
        size_t hrv_inc_samples = config.hrv_update_interval_s * frequency * ppg.axes;
        // Greater than b/c can have multiple increments (HRs) per window
        assert(signal->total_length >= floats_per_inc);

        int out_idx = 0;
        size_t hrv_count = 0;
        for (size_t i = 0; i <= signal->total_length - floats_per_inc; i += floats_per_inc) {
            // TODO ask for smaller increments and bp them into place
            // Copy into the end of the buffer
            matrix_t temp(ppg.win_inc_samples, ppg.axes);
            signal->get_data(i, floats_per_inc, temp.buffer);
            auto hr = ppg.stream(&temp);
            if (result) {
                result->hr_calcs.heart_rate = hr;
            }
            if(!hrv || (hrv && config.include_hr)) {
                output_matrix->buffer[out_idx++] = hr;
            }
            if(hrv) {
<<<<<<< HEAD
                auto ibis = ppg.get_ibis();
                hrv->add_ibis(ibis);
=======
                auto peaks = ppg.get_peaks();
                hrv->add_streaming_beats(peaks);
>>>>>>> ab9923a28fc5410c832133804f4acbe53395f851
                hrv_count += floats_per_inc;
                if(hrv_count >= hrv_inc_samples) {
                    fvec features = hrv->get_hrv_features(0);
                    for(size_t j = 0; j < features.size(); j++) {
                        output_matrix->buffer[out_idx++] = features[j];
                    }
                    hrv_count = 0;
                }
            }
        }
        return EIDSP_OK;
    }

    hr_class(ei_dsp_config_hr_t* config, float frequency)
        : ppg(frequency,
              config->axes,
              int(frequency * config->hr_win_size_s),
              int(frequency * 2), // TODO variable overlap
              config->filter_preset,
              config->acc_resting_std,
              config->sensitivity,
              true), hrv(nullptr)
    {
        auto table = config->named_axes;
        for( size_t i = 0; i < config->named_axes_size; i++ ) {
            ppg.set_offset_table(i, table[i].axis);
        }
        // if not "none"
        if(strcmp(config->hrv_features,"none") != 0) {
            // new is overloaded to use ei_malloc
            hrv = new hrv_wrap(
                frequency,
                config->hrv_features,
                config->hrv_update_interval_s,
                config->hrv_win_size_s,
                2); // TODO variable window?
        }
    }

    ~hr_class() {
        if(hrv) {
            // delete is overloaded to use ei_free
            delete hrv;
        }
    }

    float get_last_hr() {
        return ppg._res.hr;
    }

    // Boilerplate below here
    static DspHandle* create(void* config, float frequency);

    void* operator new(size_t size) {
        // Custom memory allocation logic here
        return ei_malloc(size);
    }

    void operator delete(void* ptr) {
        // Custom memory deallocation logic here
        ei_free(ptr);
    }
    // end boilerplate
private:
    ei::hr_ppg ppg;
    hrv_wrap* hrv; // pointer b/c we don't always need it
};

DspHandle* hr_class::create(void* config_in, float frequency) { // NOLINT def in header is OK at EI
    auto config = reinterpret_cast<ei_dsp_config_hr_t*>(config_in);
    return new hr_class(config, frequency);
};

/*
NOTE, contact EI sales for license and source to use EI heart rate and heart rate variance functions in deployment
*/

#endif