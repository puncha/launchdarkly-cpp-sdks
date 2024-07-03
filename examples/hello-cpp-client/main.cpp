#include <launchdarkly/client_side/client.hpp>
#include <launchdarkly/context_builder.hpp>

#include <cstring>
#include <iostream>
#include <locale>

using namespace launchdarkly;
using namespace launchdarkly::client_side;

#define MOBILE_KEY ""

char const* get_with_env_fallback(char const* source_val,
                                  char const* env_variable,
                                  char const* error_msg) {
    if (strlen(source_val)) {
        return source_val;
    }

    if (char const* from_env = std::getenv(env_variable);
        from_env && strlen(from_env)) {
        return from_env;
    }

    std::cout << "*** " << error_msg << std::endl;
    std::exit(1);
}
// Set INIT_TIMEOUT_MILLISECONDS to the amount of time you will wait for
// the client to become initialized.
#define INIT_TIMEOUT_MILLISECONDS 3000

Client* g_client = nullptr;
auto get_client() {
    return g_client;
}

auto startClient(auto context, auto& config) {
    auto client = new Client(config, context);

    auto start_result = client->StartAsync();

    if (auto const status = start_result.wait_for(
            std::chrono::milliseconds(INIT_TIMEOUT_MILLISECONDS));
        status == std::future_status::ready) {
        if (start_result.get()) {
            std::cout << "SDK is ready!" << std::endl;
            g_client = client;
        } else {
            std::cout << "*** SDK failed to initialize\n";
            throw std::runtime_error("failed to initialize");
        }
    } else {
        std::cout << "*** SDK initialization didn't complete in "
                  << INIT_TIMEOUT_MILLISECONDS << "ms\n";
        throw std::runtime_error("SDK initialization timeout");
    }
}

void keep_app_running() {
    std::cout << "Press any key to continue...\n" << std::endl;
    getchar();
}

void hello_ios_boolean_returns_false_when_first_name_is_Tom(auto config) {
    auto context = ContextBuilder()
                       .Kind("user", "user-tom-key")
                       .Name("Tom")
                       .Set("firstName", "Tom")
                       .Build();

    startClient(std::move(context), config);

    bool const flag_value =
        get_client()->BoolVariation("hello-ios-boolean", false);
    assert(false == flag_value);
}

void hello_ios_boolean_returns_true_when_first_name_is_Blue(auto config) {
    auto context = ContextBuilder()
                       .Kind("user", "user-blue-key")
                       .Name("Blue")
                       .Set("firstName", "Blue")
                       .Build();

    startClient(std::move(context), config);

    bool const flag_value =
        get_client()->BoolVariation("hello-ios-boolean", false);
    assert(true == flag_value);
}

void hello_ios_boolean_returns_true_by_default_when_first_name_is_not_set(
    auto config) {
    auto context =
        ContextBuilder().Kind("user", "user-puncha-key").Name("PunCha").Build();

    startClient(std::move(context), config);

    bool const flag_value =
        get_client()->BoolVariation("hello-ios-boolean", false);
    assert(true == flag_value);
}

void get_all_flag_should_success(auto config) {
    auto context =
        ContextBuilder().Kind("user", "user-puncha-key").Name("PunCha").Build();

    startClient(std::move(context), config);

    auto const flag_values = get_client()->AllFlags();
    assert(5 == flag_values.size());
    assert(true == flag_values.contains("hello-ios-boolean"));
    assert(true == flag_values.contains("custom_attribue_username"));
    assert(true == flag_values.contains("custom_attribue"));
    assert(true == flag_values.contains("andy_testing"));
}

void custom_attribue_is_Android_when_platform_is_Android(auto config) {
    auto context = ContextBuilder()
                       .Kind("user", "user-puncha-key")
                       .SetPrivate("platform", "Android")
                       .Build();

    startClient(std::move(context), config);

    auto const flag_value =
        get_client()->StringVariation("custom_attribue", "Dummy");
    assert("Android" == flag_value);
}

void custom_attribue_is_iOS_when_platform_is_iOS(auto config) {
    auto context = ContextBuilder()
                       .Kind("user", "user-puncha-key")
                       .SetPrivate("platform", "iOS")
                       .Build();

    startClient(std::move(context), config);

    auto const flag_value =
        get_client()->StringVariation("custom_attribue", "Dummy");
    std::cout << "custom_attribue's value is " << flag_value << std::endl;
    assert("iOS" == flag_value);
}

void custom_attribue_is_Unknown_when_platform_is_Unknown(auto config) {
    auto context = ContextBuilder()
                       .Kind("user", "user-puncha-key")
                       .SetPrivate("platform", "Dummy")
                       .Build();

    startClient(std::move(context), config);

    auto const flag_value =
        get_client()->StringVariation("custom_attribue", "Dummy");
    assert("Unknown" == flag_value);
}

void flag_value_should_reevaluated_after_identify(auto config) {
    auto context = ContextBuilder()
                       .Kind("device", "device-key-abc")
                       .Anonymous(true)
                       .Build();

    startClient(context, config);

    auto const flag_value =
        get_client()->BoolVariationDetail("hello-ios-boolean", "false");
    std::cout << "hello-ios-boolean is set to true because "
              << (flag_value.Reason().has_value()
                      ? std::to_string(static_cast<int>(
                            flag_value.Reason().value().Kind()))
                      : "<no reason>")
              << std::endl;
    assert(true == flag_value.Value());

    auto updated_context = ContextBuilder(context)
                               .Kind("device", "device-key-abc")
                               .Kind("user", "user-key-tom")
                               .Name("Tom")
                               .Set("firstName", "Tom")
                               .Build();

    auto start_result = get_client()->IdentifyAsync(updated_context);
    if (auto const status = start_result.wait_for(
            std::chrono::milliseconds(INIT_TIMEOUT_MILLISECONDS));
        status == std::future_status::ready) {
        if (!start_result.get()) {
            throw std::runtime_error("fail to identify");
        }
    } else {
        std::cout << "*** Identify didn't complete in "
                  << INIT_TIMEOUT_MILLISECONDS << "ms\n";
        throw std::runtime_error("Identify() timeout");
    }

    auto const reevaluated_flag_value =
        get_client()->BoolVariation("hello-ios-boolean", "false");
    std::cout << "hello-ios-boolean is set to false because "
              << (flag_value.Reason().has_value()
                      ? std::to_string(static_cast<int>(
                            flag_value.Reason().value().Kind()))
                      : "<no reason>")
              << std::endl;
    assert(false == reevaluated_flag_value);
}

void listener_test(auto config) {
    auto context = ContextBuilder()
                       .Kind("device", "device-key-abc")
                       .Anonymous(true)
                       .Build();

    startClient(context, config);

    auto listener = get_client()->FlagNotifier().OnFlagChange(
        "listener_test", [](auto event) {
            if (event->Deleted()) {
                std::cout << "The flag was deleted" << std::endl;
            } else {
                std::cout << "The flag was " << event->OldValue()
                          << " and now it is " << event->NewValue()
                          << std::endl;
            }
        });

    keep_app_running();
    listener->Disconnect();
}

void custom_event_test(auto config) {
    auto context =
        ContextBuilder().Kind("user", "user-puncha-key").Name("PunCha").Build();

    startClient(std::move(context), config);

    get_client()->Track("event_key_123");
    get_client()->FlushAsync();
}

[[noreturn]] void monitoring_sdk_status_test(auto config) {
    auto context =
        ContextBuilder().Kind("user", "user-puncha-key").Name("PunCha").Build();

    startClient(std::move(context), config);

    get_client()->DataSourceStatus().OnDataSourceStatusChange(
        [](data_sources::DataSourceStatus const& status) {
            std::cout << "SDK Status: " << status.State() << std::endl;
        });

    keep_app_running();
}

[[noreturn]] void offline_test(auto config) {
    while(true) {
        custom_attribue_is_iOS_when_platform_is_iOS(config);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    keep_app_running();
}

void run_all_cases(auto config) {
    hello_ios_boolean_returns_false_when_first_name_is_Tom(config);
    hello_ios_boolean_returns_true_when_first_name_is_Blue(config);
    hello_ios_boolean_returns_true_by_default_when_first_name_is_not_set(config);

//    get_all_flag_should_success(config);
//
//    custom_attribue_is_Android_when_platform_is_Android(config);
//    custom_attribue_is_iOS_when_platform_is_iOS(config);
//    custom_attribue_is_Unknown_when_platform_is_Unknown(config);
//
//    flag_value_should_reevaluated_after_identify(config);

    // listener_test(config);
    //
    // custom_event_test(config);
    //
    // monitoring_sdk_status_test(config);
    //
    // offline_test(config);
}

int main() {
    char const* mobile_key = get_with_env_fallback(
        MOBILE_KEY, "LD_MOBILE_KEY",
        "Please specify your LaunchDarkly key first.\n\nAlternatively, "
        "set the LD_MOBILE_KEY environment variable.\n");

    auto config_builder = ConfigBuilder(mobile_key);
    config_builder.Persistence().None();
    config_builder.DataSource().WithReasons(true);
    // config_builder.Offline(true);
    auto config = config_builder.Build();
    if (!config) {
        std::cout << "error: config is invalid: " << config.error() << '\n';
        return 1;
    }

    run_all_cases(std::move(config.value()));

    return 0;
}
