#include "event.h"

#include "app_config.h"

//====================================================
// EVENT PROCESS
//====================================================

void event_process(

    const air_processed_data_t *data,

    air_event_t *event)
{
    switch (event->state) {

        //--------------------------------------------
        // IDLE
        //--------------------------------------------

        case EVENT_IDLE:

            if (data->pm25_spike >
                EVENT_PM_START_THRESHOLD ||

                data->voc_spike >
                EVENT_VOC_START_THRESHOLD) {

                event->state =
                    EVENT_SUSPECT;

                event->duration = 0;
            }

        break;

        //--------------------------------------------
        // SUSPECT
        //--------------------------------------------

        case EVENT_SUSPECT:

            event->duration++;

            if (data->pm25_spike >
                EVENT_PM_ACTIVE_THRESHOLD ||

                data->voc_spike >
                EVENT_VOC_ACTIVE_THRESHOLD) {

                event->state =
                    EVENT_ACTIVE;
            }

            if (data->pm25_spike <
                EVENT_PM_END_THRESHOLD &&

                data->voc_spike <
                EVENT_VOC_END_THRESHOLD) {

                event->state =
                    EVENT_IDLE;
            }

        break;

        //--------------------------------------------
        // ACTIVE
        //--------------------------------------------

        case EVENT_ACTIVE:

            event->duration++;

            //----------------------------------------
            // PEAKS
            //----------------------------------------

            if (data->pm25 >
                event->peak_pm25)

                event->peak_pm25 =
                    data->pm25;

            if (data->voc >
                event->peak_voc)

                event->peak_voc =
                    data->voc;

            if (data->nox >
                event->peak_nox)

                event->peak_nox =
                    data->nox;

            //----------------------------------------
            // DECAY
            //----------------------------------------

            if (data->pm25_spike <
                EVENT_PM_END_THRESHOLD &&

                data->voc_spike <
                EVENT_VOC_END_THRESHOLD) {

                event->state =
                    EVENT_DECAY;
            }

        break;

        //--------------------------------------------
        // DECAY
        //--------------------------------------------

        case EVENT_DECAY:

            if (data->pm25_spike <
                EVENT_PM_IDLE_THRESHOLD &&

                data->voc_spike <
                EVENT_VOC_IDLE_THRESHOLD) {

                event->state =
                    EVENT_IDLE;
            }

        break;
    }
}