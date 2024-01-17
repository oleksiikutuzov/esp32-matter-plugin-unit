/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>

#include <app_priv.h>
#include <app_reset.h>

#include <board_config.h>

#include "driver/gpio.h"

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

static const char *TAG = "app_main";
uint16_t plugin_unit_endpoint_id_1 = 0;
uint16_t plugin_unit_endpoint_id_2 = 0;
uint16_t plugin_unit_endpoint_id_3 = 0;
uint16_t plugin_unit_endpoint_id_4 = 0;

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type)
    {
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "Interface IP Address Changed");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;

    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "Commissioning session started");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
        ESP_LOGI(TAG, "Commissioning session stopped");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "Commissioning window opened");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
        ESP_LOGI(TAG, "Commissioning window closed");
        break;

    default:
        break;
    }
}

// This callback is invoked when clients interact with the Identify Cluster.
// In the callback implementation, an endpoint can identify itself. (e.g., by flashing an LED or light).
static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

// This callback is called for every attribute update. The callback implementation shall
// handle the desired attributes and return an appropriate error code. If the attribute
// is not of your interest, please do not return an error code and strictly return ESP_OK.
static esp_err_t app_attribute_update_cb(callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    if (type == PRE_UPDATE)
    {
        /* Handle the attribute updates here. */
        bool new_state = val->val.b;
        ESP_LOGE(TAG, "SET Endpoint %d to %d", endpoint_id, new_state);
        if (endpoint_id == plugin_unit_endpoint_id_1)
        {
            gpio_set_level(CHANNEL_1_PIN, new_state);
            ESP_LOGE(TAG, "SET CHANNEL 1 to %d", new_state);
        }
        else if (endpoint_id == plugin_unit_endpoint_id_2)
        {
            gpio_set_level(CHANNEL_2_PIN, new_state);
            ESP_LOGE(TAG, "SET CHANNEL 2 to %d", new_state);
        }
        else if (endpoint_id == plugin_unit_endpoint_id_3)
        {
            gpio_set_level(CHANNEL_3_PIN, new_state);
            ESP_LOGE(TAG, "SET CHANNEL 3 to %d", new_state);
        }
        else if (endpoint_id == plugin_unit_endpoint_id_4)
        {
            gpio_set_level(CHANNEL_4_PIN, new_state);
            ESP_LOGE(TAG, "SET CHANNEL 4 to %d", new_state);
        }
    }

    return ESP_OK;
}

extern "C" void app_main()
{
    esp_err_t err = ESP_OK;

    /* Initialize the ESP NVS layer */
    nvs_flash_init();

    gpio_set_direction(CHANNEL_1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHANNEL_2_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHANNEL_3_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHANNEL_4_PIN, GPIO_MODE_OUTPUT);

    /* Initialize driver */
    app_driver_handle_t switch_handle = app_driver_switch_init();
    app_reset_button_register(switch_handle);

    /* Create a Matter node and add the mandatory Root Node device type on endpoint 0 */
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);

    on_off_plugin_unit::config_t plugin_unit_config;
    plugin_unit_config.on_off.on_off = false;
    plugin_unit_config.on_off.lighting.start_up_on_off = false;
    endpoint_t *endpoint_1 = on_off_plugin_unit::create(node, &plugin_unit_config,
                                                        ENDPOINT_FLAG_NONE, NULL);
    endpoint_t *endpoint_2 = on_off_plugin_unit::create(node, &plugin_unit_config,
                                                        ENDPOINT_FLAG_NONE, NULL);
    endpoint_t *endpoint_3 = on_off_plugin_unit::create(node, &plugin_unit_config,
                                                        ENDPOINT_FLAG_NONE, NULL);
    endpoint_t *endpoint_4 = on_off_plugin_unit::create(node, &plugin_unit_config,
                                                        ENDPOINT_FLAG_NONE, NULL);

    /* These node and endpoint handles can be used to create/add other endpoints and clusters. */
    if (!node || !endpoint_1 || !endpoint_2 || !endpoint_3 || !endpoint_4)
    {
        ESP_LOGE(TAG, "Matter node creation failed");
    }

    /* Add group cluster to the switch endpoint */
    cluster::groups::config_t groups_config;
    cluster::groups::create(endpoint_1, &groups_config, CLUSTER_FLAG_SERVER | CLUSTER_FLAG_CLIENT);
    cluster::groups::create(endpoint_2, &groups_config, CLUSTER_FLAG_SERVER | CLUSTER_FLAG_CLIENT);
    cluster::groups::create(endpoint_3, &groups_config, CLUSTER_FLAG_SERVER | CLUSTER_FLAG_CLIENT);
    cluster::groups::create(endpoint_4, &groups_config, CLUSTER_FLAG_SERVER | CLUSTER_FLAG_CLIENT);

    plugin_unit_endpoint_id_1 = endpoint::get_id(endpoint_1);
    ESP_LOGI(TAG, "Switch 1 created with endpoint_id %d", plugin_unit_endpoint_id_1);
    plugin_unit_endpoint_id_2 = endpoint::get_id(endpoint_2);
    ESP_LOGI(TAG, "Switch 2 created with endpoint_id %d", plugin_unit_endpoint_id_2);
    plugin_unit_endpoint_id_3 = endpoint::get_id(endpoint_3);
    ESP_LOGI(TAG, "Switch 3 created with endpoint_id %d", plugin_unit_endpoint_id_3);
    plugin_unit_endpoint_id_4 = endpoint::get_id(endpoint_4);
    ESP_LOGI(TAG, "Switch 4 created with endpoint_id %d", plugin_unit_endpoint_id_4);

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    /* Set OpenThread platform config */
    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };
    set_openthread_platform_config(&config);
#endif

    /* Matter start */
    err = esp_matter::start(app_event_cb);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Matter start failed: %d", err);
    }

#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::init();
#endif
}
