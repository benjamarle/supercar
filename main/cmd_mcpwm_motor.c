/* cmd_mcpwm_motor.h

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_log.h"
#include "supercar_main.h"

#define MOTOR_CTRL_CMD_CHECK(ins)   if(arg_parse(argc, argv, (void **)&ins)){ \
                                        arg_print_errors(stderr, ins.end, argv[0]); \
                                        return 0;}

static supercar_motor_control_t *mc;
SemaphoreHandle_t g_motor_mux;

static struct {
    struct arg_int *period;
    struct arg_lit *show;
    struct arg_end *end;

} motor_ctrl_config_args;

static struct {
    struct arg_dbl *max;
    struct arg_dbl *min;
    struct arg_dbl *pace;
    struct arg_dbl *init;
    struct arg_end *end;

} motor_ctrl_expt_args;

static struct {
    struct arg_int *start;
    struct arg_lit *stop;
    struct arg_lit *reverse;
    struct arg_end *end;
} motor_ctrl_motor_args;


static void print_current_status(void)
{
    printf("\n -----------------------------------------------------------------\n");
    printf("                  Current Configuration Status                \n\n");
    printf(" Configuration\n       Period = %d ms\n\n",
           mc->cfg.ctrl_period);
    printf(" Expectation \n       init = %.3f\tmax = %.3f\tmin = %.3f\tpace = %.3f\n\n",
           mc->cfg.expt_init, mc->cfg.expt_max, mc->cfg.expt_min, mc->cfg.expt_pace);
    printf(" MCPWM\n       Frequency = %d Hz\n\n", mc->cfg.pwm_freq);
    printf(" -----------------------------------------------------------------\n\n");
}


static int do_motor_ctrl_config_cmd(int argc, char **argv)
{
    MOTOR_CTRL_CMD_CHECK(motor_ctrl_config_args);
    xSemaphoreTake(g_motor_mux, portMAX_DELAY);
    if (motor_ctrl_config_args.period->count) {
        mc->cfg.ctrl_period = motor_ctrl_config_args.period->ival[0];
        printf("config: control period = mc->cfg.ctrl_period\n");
    }

    if (motor_ctrl_config_args.show->count) {
        print_current_status();
    }
    xSemaphoreGive(g_motor_mux);
    return 0;
}

static int do_motor_ctrl_expt_cmd(int argc, char **argv)
{
    MOTOR_CTRL_CMD_CHECK(motor_ctrl_expt_args);
    xSemaphoreTake(g_motor_mux, portMAX_DELAY);
    if (motor_ctrl_expt_args.init->count) {
        mc->cfg.expt_init = motor_ctrl_expt_args.init->dval[0];
        printf("expt: init = %.3f\n", mc->cfg.expt_init);
    }
    if (motor_ctrl_expt_args.max->count) {
        mc->cfg.expt_max = motor_ctrl_expt_args.max->dval[0];
        printf("expt: max = %.3f\n", mc->cfg.expt_max);
    }
    if (motor_ctrl_expt_args.min->count) {
        mc->cfg.expt_min = motor_ctrl_expt_args.min->dval[0];
        printf("expt: min = %.3f\n", mc->cfg.expt_min);
    }
    if (motor_ctrl_expt_args.pace->count) {
        mc->cfg.expt_pace = motor_ctrl_expt_args.pace->dval[0];
        printf("expt: pace = %.3f\n", mc->cfg.expt_pace);
    }
    xSemaphoreGive(g_motor_mux);
    return 0;
}

static int do_motor_ctrl_motor_cmd(int argc, char **argv)
{
    MOTOR_CTRL_CMD_CHECK(motor_ctrl_motor_args);
    xSemaphoreTake(g_motor_mux, portMAX_DELAY);
    if (motor_ctrl_motor_args.start->count) {
       
        mc->expt = mc->cfg.expt_max;
        // Start the motor
        brushed_motor_start(mc);
        printf("motor: motor starts to run, input 'motor -d' to stop it\n");
    }

    if (motor_ctrl_motor_args.stop->count) {
        // Stop the motor
        brushed_motor_stop(mc);
        printf("motor: motor stoped\n");
    }

    if(motor_ctrl_motor_args.reverse->count) {
        brushed_motor_set_direction(mc, mc->direction == MOTOR_RIGHT ? MOTOR_LEFT : MOTOR_RIGHT);
        printf("motor: motor reversed -> %s\n", mc->direction == MOTOR_RIGHT ? "LEFT" : "RIGHT");
    }
    xSemaphoreGive(g_motor_mux);
    return 0;
}

static void register_motor_ctrl_config(void)
{
    motor_ctrl_config_args.period = arg_int0("T", "period", "<ms>", "Set motor control period");
    motor_ctrl_config_args.show = arg_lit0("s", "show", "Show current configurations");
    motor_ctrl_config_args.end = arg_end(2);
    const esp_console_cmd_t motor_ctrl_cfg_cmd = {
        .command = "config",
        .help = "Enable or disable PID and set motor control period",
        .hint = "config -s",
        .func = &do_motor_ctrl_config_cmd,
        .argtable = &motor_ctrl_config_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&motor_ctrl_cfg_cmd));
}

static void register_motor_ctrl_expt(void)
{
    motor_ctrl_expt_args.init = arg_dbl0("i", "init", "<duty>", "Initial expectation. Usually between -100~100");
    motor_ctrl_expt_args.max  = arg_dbl0(NULL, "max", "<duty>", "Max limitation for dynamic expectation");
    motor_ctrl_expt_args.min  = arg_dbl0(NULL, "min", "<duty>", "Min limitation for dynamic expectation");
    motor_ctrl_expt_args.pace = arg_dbl0("p", "pace", "<double>", "The increasing pace of expectation every 50ms");
    motor_ctrl_expt_args.end  = arg_end(2);

    const esp_console_cmd_t motor_ctrl_expt_cmd = {
        .command = "expt",
        .help = "Set initial value, limitation and wave mode of expectation. Both dynamic and static mode are available",
        .hint = "expt -i <duty> -m <fixed/tri/rect> -p <double> --max <duty> --min <duty>",
        .func = &do_motor_ctrl_expt_cmd,
        .argtable = &motor_ctrl_expt_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&motor_ctrl_expt_cmd));
}

static void register_motor_ctrl_motor(void)
{
    motor_ctrl_motor_args.start = arg_int0("u", "start", "<ms>", "Set running seconds for motor, set '0' to keep motor running");
    motor_ctrl_motor_args.stop = arg_lit0("d", "stop", "Stop the motor");
    motor_ctrl_motor_args.reverse = arg_lit0("r", "reverse", "Reverse the motor");
    motor_ctrl_motor_args.end = arg_end(2);

    const esp_console_cmd_t motor_ctrl_motor_cmd = {
        .command = "motor",
        .help = "Start or stop the motor",
        .hint = "motor -u 10",
        .func = &do_motor_ctrl_motor_cmd,
        .argtable = &motor_ctrl_motor_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&motor_ctrl_motor_cmd));
}

void cmd_mcpwm_motor_init(supercar_motor_control_t *motor_ctrl)
{
    mc = motor_ctrl;
    g_motor_mux = motor_ctrl->mutex;
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "mcpwm-motor>";

    // install console REPL environment
#if CONFIG_ESP_CONSOLE_UART
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
#elif CONFIG_ESP_CONSOLE_USB_CDC
    esp_console_dev_usb_cdc_config_t cdc_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&cdc_config, &repl_config, &repl));
#endif

    register_motor_ctrl_config();
    register_motor_ctrl_expt();
    register_motor_ctrl_motor();

    printf("\n =================================================================\n");
    printf(" |             Example of Motor Control                          |\n");
    printf(" |                                                               |\n");
    printf(" |  1. Try 'help', check all supported commands                  |\n");
    printf(" |  2. Try 'config' to set control period or pwm frequency       |\n");
    printf(" |  3. Try 'pid' to configure pid paremeters                     |\n");
    printf(" |  4. Try 'expt' to set expectation value and mode              |\n");
    printf(" |  5. Try 'motor' to start motor in several seconds or stop it  |\n");
    printf(" |                                                               |\n");
    printf(" =================================================================\n\n");

    printf("Default configuration are shown as follows.\nYou can input 'config -s' to check current status.");
    print_current_status();

    // start console REPL
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
