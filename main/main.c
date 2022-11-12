#include "esp_event.h"
#include "driver/gpio.h"
#include "soc/gpio_reg.h"

#define LedRojo		GPIO_NUM_21
#define LedVerde	GPIO_NUM_22
#define LedAzul		GPIO_NUM_23
#define Switch 		GPIO_NUM_19
#define ESP_INTR_FLAG_DEFAULT 0

uint32_t Count, BitMask, DataPinsMask;

static void IRAM_ATTR gpio_isr_handler(void* arg){
	DataPinsMask = Count << 21;
	BitMask = REG_READ(GPIO_OUT_REG);
	BitMask &= (~0x00E00000);
	BitMask |= DataPinsMask;
    REG_WRITE(GPIO_OUT_REG, BitMask);
    Count++;
    Count &=0x07;
    //vTaskDelay(100 / portTICK_PERIOD_MS);
}

void app_main(void){
	gpio_config_t io_conf;
		io_conf.pin_bit_mask = (1ULL<<LedRojo | 1ULL<<LedVerde | 1ULL<<LedAzul);		// GPIO21, GPIO22, GPIO23
		io_conf.mode = GPIO_MODE_OUTPUT;     			// GPIO_MODE_DISABLE, GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, GPIO_MODE_OUTPUT_OD,
		                                           	   	// GPIO_MODE_INPUT_OUTPUT_OD, GPIO_MODE_INPUT_OUTPUT
		io_conf.pull_up_en = GPIO_PULLUP_DISABLE;       // GPIO_PULLUP_ENABLE
		io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;   // GPIO_PULLDOWN_ENABLE
		io_conf.intr_type = GPIO_PIN_INTR_DISABLE;      // GPIO_INTR_POSEDGE, GPIO_INTR_NEGEDGE, GPIO_INTR_ANYEDG, GPIO_INTR_LOW_LEVEL, GPIO_INTR_HIGH_LEVEL
    gpio_config(&io_conf);

    gpio_set_direction(Switch, GPIO_MODE_INPUT);
    gpio_set_pull_mode(Switch, GPIO_PULLUP_ONLY);  		// GPIO_PULLUP_ONLY, GPIO_PULLDOWN_ONLY, GPIO_PULLUP_PULLDOWN, GPIO_FLOATING
    gpio_set_intr_type(Switch, GPIO_PIN_INTR_NEGEDGE); // Configura el tipo de interrupcion del pin
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(Switch, gpio_isr_handler, (void*) Switch);
    Count = 0;
    REG_WRITE(GPIO_OUT_REG, 0);
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
