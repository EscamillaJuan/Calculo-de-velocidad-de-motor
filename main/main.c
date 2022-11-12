#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pcnt.h"
#include "driver/mcpwm.h"
#include "esp_err.h"
#include "soc/mcpwm_periph.h"
#include "esp_event.h"
#include "esp_sleep.h"
#include <stdio.h>
#include "driver/periph_ctrl.h"
#include "soc/rtc.h"
#include "esp_attr.h"
#include "esp_intr_alloc.h"


#define PCNT_HIGH_LIMIT 	10
#define PCNT_LOW_LIMIT  	0
#define GPIO_CLK 			19
#define GPIO_DT 			18
#define out_a 				22
#define out_b				23


#define Capture0 			2
#define CAP0_INT_EN 		BIT(27)


mcpwm_capture_signal_t current_cap_value, previous_cap_value, T_Diff;
int NuevaCaptura;
static mcpwm_dev_t *MCPWM[2] = {&MCPWM0, &MCPWM1};
float Frecuencia, Velocidad;
static void IRAM_ATTR isr_handler( void *arg)
{
	uint32_t mcpwm_intr_status;
	mcpwm_intr_status = MCPWM[0]->int_st.val;
	if(mcpwm_intr_status & CAP0_INT_EN){
		current_cap_value = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP0);
		T_Diff = (current_cap_value - previous_cap_value) / (rtc_clk_apb_freq_get() / 1000000);
		previous_cap_value = current_cap_value;
	}
	MCPWM[0]->int_clr.val = mcpwm_intr_status;
	NuevaCaptura = 1;
}


void app_main(void){

	mcpwm_config_t pwm_config;
		pwm_config.frequency = 2000;
		pwm_config.cmpr_a = 50;
		pwm_config.cmpr_b = 50;
		pwm_config.counter_mode = MCPWM_UP_COUNTER;
		pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
		mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, out_a);
	mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, out_b);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, Capture0);
    gpio_pulldown_en(Capture0);

    mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, MCPWM_POS_EDGE, 0);
    MCPWM[0]->int_ena.val = CAP0_INT_EN;
    mcpwm_isr_register(MCPWM_UNIT_0, isr_handler, NULL, ESP_INTR_FLAG_IRAM, NULL);


    pcnt_config_t cnt0_ch0_config = {
        .pulse_gpio_num = GPIO_CLK,
		.ctrl_gpio_num = GPIO_DT,
		.unit = PCNT_UNIT_0,
		.channel = PCNT_CHANNEL_0,
		.pos_mode = PCNT_CHANNEL_EDGE_ACTION_DECREASE,
		.neg_mode = PCNT_CHANNEL_EDGE_ACTION_HOLD,
		.lctrl_mode = PCNT_CHANNEL_LEVEL_ACTION_KEEP,
		.hctrl_mode = PCNT_CHANNEL_LEVEL_ACTION_INVERSE,
		.counter_h_lim = PCNT_HIGH_LIMIT,
		.counter_l_lim = PCNT_LOW_LIMIT
    };
    pcnt_unit_config(&cnt0_ch0_config);
    pcnt_set_filter_value(PCNT_UNIT_0, 1000);
    pcnt_filter_enable(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    int16_t Contador;
    float CicloDeTrabajo = 0;
    int old_contador;
    while (1) {
    	old_contador = Contador;
    	pcnt_get_counter_value(PCNT_UNIT_0, &Contador);
    	if(Contador-old_contador==1){
    		CicloDeTrabajo = CicloDeTrabajo + 10;
    		CicloDeTrabajo = CicloDeTrabajo==100?100:CicloDeTrabajo;
    	}
    	else if(Contador-old_contador==-1){
    		CicloDeTrabajo = CicloDeTrabajo - 10;
    		CicloDeTrabajo = CicloDeTrabajo<0?0:CicloDeTrabajo;
    	}

    	if (NuevaCaptura){
    		Frecuencia = (1E06/T_Diff);
    		Velocidad = (60 / 20) * (Frecuencia);
    		printf("Velocidad:%0.2f RPM\n", Velocidad);
    		NuevaCaptura = 0;
    	}
    	mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_GEN_B, CicloDeTrabajo);
    	vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
