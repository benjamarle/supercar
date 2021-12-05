/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "supercar_main.h"
#include "supercar_config.h"

static const char *REST_TAG = "esp-rest";
#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
    supercar_t* car;
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "image/svg+xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(REST_TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(REST_TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(REST_TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static void supercar_add_motor_json(cJSON* node, const char* name, supercar_motor_control_t* mctl){
    cJSON* motor_json = cJSON_AddObjectToObject(node, name);
    cJSON_AddNumberToObject(motor_json, "start_time", mctl->start_time);
    cJSON_AddBoolToObject(motor_json, "start_flag", mctl->start_flag);
    cJSON_AddNumberToObject(motor_json, "duty_cycle", mctl->duty_cycle);
    cJSON_AddStringToObject(motor_json, "direction", mctl->direction == MOTOR_LEFT ? "LEFT" : "RIGHT");
    cJSON_AddNumberToObject(motor_json, "expt", mctl->expt);
    cJSON_AddStringToObject(motor_json, "name", mctl->name);
    cJSON* cfg = cJSON_AddObjectToObject(motor_json, "cfg");
   supercar_serialize_motor_config(cfg, mctl);
}


/* Simple handler for getting system handler */
static void supercar_serialize(cJSON* node, supercar_t* car)
{
    cJSON_AddBoolToObject(node, "power", car->power);
    supercar_add_motor_json(node, "propulsion_motor_ctrl", &car->propulsion_motor_ctrl);
    supercar_add_motor_json(node, "steering_motor_ctrl", &car->steering_motor_ctrl);
    cJSON_AddStringToObject(node, "mode", supercar_get_mode(car) == MOTION ? "MOTION" : "SWAY");
    cJSON_AddStringToObject(node, "applied_mode", car->applied_mode == MOTION ? "MOTION" : "SWAY");
    cJSON_AddStringToObject(node, "control_type", car->control_type == LOCAL ? "LOCAL" : "REMOTE");
    cJSON_AddStringToObject(node, "steering", car->steering == STEER_NONE ? "NONE" : (car->steering == STEER_LEFT ? "LEFT" : "RIGHT"));
    cJSON_AddBoolToObject(node, "reverse_direction", car->reverse_direction);
    cJSON_AddBoolToObject(node, "reverse_mode", car->reverse_mode);
    cJSON_AddStringToObject(node, "running", car->running == DIRECTION_NONE ? "NONE" : (car->running == DIRECTION_FORWARD ? "FORWARD" : "BACKWARD"));
    cJSON* distance = cJSON_AddObjectToObject(node, "distance");
    cJSON_AddNumberToObject(distance, "front_left", car->distance.front_left);
    cJSON_AddNumberToObject(distance, "front_right", car->distance.front_right);
    cJSON_AddNumberToObject(distance, "back_left", car->distance.back_left);
    cJSON_AddNumberToObject(distance, "back_right", car->distance.back_right);
    cJSON* cfg = cJSON_AddObjectToObject(node, "cfg");
    supercar_serialize_config(cfg, car);
}

static esp_err_t supercar_generic_get_handler(httpd_req_t *req, void (*serialize)(cJSON*, supercar_t*)){
    rest_server_context_t* ctx = req->user_ctx;
    supercar_t* car = ctx->car;
    httpd_resp_set_type(req, "application/json");
    cJSON* cfg = cJSON_CreateObject();
    
    serialize(cfg, car);
    const char *config_json = cJSON_Print(cfg);
    httpd_resp_sendstr(req, config_json);
    free((void *)config_json);
    cJSON_Delete(cfg);
    return ESP_OK;
}

static esp_err_t supercar_generic_put_handler(httpd_req_t *req, void (*deserialize)(cJSON*, supercar_t*), esp_err_t (*save)(supercar_t*)){

       /* Destination buffer for content of HTTP POST request.
     * httpd_req_recv() accepts char* only, but content could
     * as well be any binary data (needs type casting).
     * In case of string data, null termination will be absent, and
     * content length would give length of string */
    char content[256];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = req->content_len < sizeof(content) ? req->content_len : sizeof(content);

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }

    rest_server_context_t* ctx = req->user_ctx;
    supercar_t* car = ctx->car;
    httpd_resp_set_type(req, "application/json");
    cJSON* cfg = cJSON_Parse(content);
    if(cfg == NULL){
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Could not parse the request");

        return ESP_FAIL;
    }
    
    deserialize(cfg, car);
    esp_err_t save_res = save(car);
    if(save_res != ESP_OK){
        cJSON_Delete(cfg);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Could not save the new parameters");
        return save_res;
    }
    const char *config_json = cJSON_Print(cfg);
    httpd_resp_sendstr(req, config_json);
    free((void *)config_json);
    cJSON_Delete(cfg);
    return ESP_OK;
}

static esp_err_t supercar_get_handler(httpd_req_t* req){
    return supercar_generic_get_handler(req, supercar_serialize);
}

static esp_err_t supercar_get_config_handler(httpd_req_t *req)
{
    return supercar_generic_get_handler(req, supercar_serialize_config);
}

static esp_err_t supercar_put_config_handler(httpd_req_t* req){
    return supercar_generic_put_handler(req, supercar_deserialize_config, supercar_config_save);
}

static esp_err_t supercar_get_propulsion_config_handler(httpd_req_t *req)
{
    return supercar_generic_get_handler(req, supercar_serialize_propulsion_config);
}

static esp_err_t supercar_put_propulsion_config_handler(httpd_req_t* req){
    return supercar_generic_put_handler(req, supercar_deserialize_propulsion_config, supercar_propulsion_config_save);
}

static esp_err_t supercar_get_steering_config_handler(httpd_req_t* req){
    return supercar_generic_get_handler(req, supercar_serialize_steering_config);
}

static esp_err_t supercar_put_steering_config_handler(httpd_req_t* req){
    return supercar_generic_put_handler(req, supercar_deserialize_steering_config, supercar_steering_config_save);
}

static void register_generic(httpd_handle_t server, const char* url, esp_err_t (*handler)(httpd_req_t* req), 
rest_server_context_t *rest_context, httpd_method_t method){
     /* URI handler for fetching system info */
    httpd_uri_t supercar_info_get_uri = {
        .uri = url,
        .method = method,
        .handler = handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &supercar_info_get_uri);
}

esp_err_t start_rest_server(const char *base_path, supercar_t* car)
{
    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));
    rest_context->car = car;

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    register_generic(server, "/api/supercar", supercar_get_handler, rest_context, HTTP_GET);
    register_generic(server, "/api/supercar/config", supercar_get_config_handler, rest_context, HTTP_GET);
    register_generic(server, "/api/supercar/config", supercar_put_config_handler, rest_context, HTTP_PUT);
    register_generic(server, "/api/supercar/propulsion/config", supercar_get_propulsion_config_handler, rest_context, HTTP_GET);
    register_generic(server, "/api/supercar/propulsion/config", supercar_put_propulsion_config_handler, rest_context, HTTP_PUT);
    register_generic(server, "/api/supercar/steering/config", supercar_get_steering_config_handler, rest_context, HTTP_GET);
    register_generic(server, "/api/supercar/steering/config", supercar_put_steering_config_handler, rest_context, HTTP_PUT);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &common_get_uri);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}
