#ifndef RPC_Callback_h
#define RPC_Callback_h

// Local includes.
#include "Callback.h"
#include "Constants.h"

// Third-party includes.
#include <ArduinoJson.h>


/// @brief Server-side RPC callback wrapper,
/// contains the needed configuration settings to create the request that should be sent to the server.
/// Documentation about the specific use of Server-side RPC in ThingsBoard can be found here https://thingsboard.io/docs/user-guide/rpc/#server-side-rpc
class RPC_Callback : public Callback<void, ArduinoJson::JsonVariantConst const &, ArduinoJson::JsonDocument &> {
    public:
        /// @brief Constructs empty callback, will result in never being called. Internals are simply default constructed as nullptr
        RPC_Callback() = default;

        /// @brief Constructs callback that will be called upon server-side RPC request arrival with the given method name
        /// @param method_name Non owning pointer to the name we expect to be sent with the server-side RPC request so that this method callback will be executed.
        /// Additionally it has to be kept alive by the user for the lifetime of this server-side RPC callback, otherwise the callback method will never be called
        /// @param callback callback method that will be called upon data arrival with the given data that was received.
        /// If nullptr is passed the callback will never be called and instead return with a defaulted instance of the requested return variable.
        /// Sometimes the server-side RPC requests expectes a response that should be sent to the server, but can be empty if that is not the case as well.
        /// See https://arduinojson.org/v7/api/jsondocument/ for more information on how to enter data into a JsonDocument
        RPC_Callback(char const * method_name, function callback)
            : Callback(callback)
            , m_method_name(method_name)
        {
                // Nothing to do
        }

        ~RPC_Callback() override = default;

        /// @brief Gets the name we expect to be sent with the server-side RPC request so that this method callback will be executed
        /// @return Non owning pointer to the name we expect to be sent with the server-side RPC request.
        /// Owned by the user that passed it originally in the constructor or with the @ref Set_Name method
        char const * Get_Name() const {
                return m_method_name;
        }

        /// @brief Sets the name we expect to be sent with the server-side RPC request so that this method callback will be executed
        /// @param method_name Non owning pointer to the name we expect to be sent with the server-side RPC request.
        /// Additionally it has to be kept alive by the user for the lifetime of this server-side RPC callback, otherwise the callback method will never be called
        void Set_Name(char const * method_name) {
                m_method_name = method_name;
        }

    private:
        char const *m_method_name = {};  // Method name
};

#endif // RPC_Callback_h
