// NOTE: The contents of this file will only be executed if
// you uncomment its entry in "web/static/js/app.js".

// To use Phoenix channels, the first step is to import Socket
// and connect at the socket path in "lib/my_app/endpoint.ex":
import {Socket} from "phoenix"

let socket = new Socket("/socket", {params: {token: window.userToken}})

// When you connect, you'll often need to authenticate the client.
// For example, imagine you have an authentication plug, `MyAuth`,
// which authenticates the session and assigns a `:current_user`.
// If the current user exists you can assign the user's token in
// the connection for use in the layout.
//
// In your "web/router.ex":
//
//     pipeline :browser do
//       ...
//       plug MyAuth
//       plug :put_user_token
//     end
//
//     defp put_user_token(conn, _) do
//       if current_user = conn.assigns[:current_user] do
//         token = Phoenix.Token.sign(conn, "user socket", current_user.id)
//         assign(conn, :user_token, token)
//       else
//         conn
//       end
//     end
//
// Now you need to pass this token to JavaScript. You can do so
// inside a script tag in "web/templates/layout/app.html.eex":
//
//     <script>window.userToken = "<%= assigns[:user_token] %>";</script>
//
// You will need to verify the user token in the "connect/2" function
// in "web/channels/user_socket.ex":
//
//     def connect(%{"token" => token}, socket) do
//       # max_age: 1209600 is equivalent to two weeks in seconds
//       case Phoenix.Token.verify(socket, "user socket", token, max_age: 1209600) do
//         {:ok, user_id} ->
//           {:ok, assign(socket, :user, user_id)}
//         {:error, reason} ->
//           :error
//       end
//     end
//
// Finally, pass the token to the Socket constructor as above.
// Or, remove it from the constructor if you don't care about
// authentication.

socket.connect()


function msg_heartbeat(messagesContainer, payload) {

  let full_div = `<div class="row console-callout console-callout-heartbeat"><div class="col-md-1"><span class="glyphicon glyphicon-heart">&nbsp;</span></div><div class="col-md-10"><h4>Heartbeat</h4><p>${payload.timestamp}</p></div></div>`;

  messagesContainer.insertAdjacentHTML("afterbegin", full_div)

}

function msg_announce_sensor_observe(messagesContainer, payload) {

    let full_div = `<div class="row console-callout console-callout-observe">
        <div class="col-md-1"><span class="glyphicon glyphicon-eye-open">&nbsp;</span></div>
        <div class="col-md-10">
            <h4>Sensor Observation Level Change ( ${payload.old_level} ➡ ${payload.new_level})</h4>

            <dl class="dl-horizontal">
                <dt>timestamp</dt><dd>${payload.timestamp}</dd>
                <dt>address</dt><dd>${payload.sensor.address}</dd>
                <dt>sensor</dt><dd>${payload.sensor.sensor_name}</dd>
                <dt>os</dt><dd>${payload.sensor.sensor_os}</dd>
                <dt>Old Level</dt><dd>${payload.old_level}</dd>
                <dt>New Level</dt><dd>${payload.new_level}</dd>
            </dl>
        </div>
    </div>`

  messagesContainer.insertAdjacentHTML("afterbegin", full_div)

}

function msg_announce_sensor_reg(messagesContainer, payload) {


    let full_div = `<div class="row console-callout console-callout-sensor">
        <div class="col-md-1"><span class="glyphicon glyphicon-zoom-in">&nbsp;</span></div>
        <div class="col-md-10">
            <h4>Sensor Registration</h4>
            <dl class="dl-horizontal">
                <dt>timestamp</dt><dd>${payload.timestamp}</dd>
                <dt>address</dt><dd>${payload.sensor.address}</dd>
                <dt>sensor</dt><dd>${payload.sensor.sensor_name}</dd>
                <dt>os</dt><dd>${payload.sensor.sensor_os}</dd>
            </dl>
        </div>
    </div>`

  messagesContainer.insertAdjacentHTML("afterbegin", full_div)

}

function msg_announce_sensor_dereg(messagesContainer, payload) {

    let full_div = `<div class="row console-callout console-callout-sensor">
        <div class="col-md-1"><span class="glyphicon glyphicon-zoom-out">&nbsp;</span></div>
        <div class="col-md-10">
            <h4>Sensor De-registration</h4>
            <dl class="dl-horizontal">
                <dt>timestamp</dt><dd>${payload.timestamp}</dd>
                <dt>address</dt><dd>${payload.sensor.address}</dd>
                <dt>sensor</dt><dd>${payload.sensor.sensor_name}</dd>
                <dt>os</dt><dd>${payload.sensor.sensor_os}</dd>
            </dl>
        </div>
    </div>`

  messagesContainer.insertAdjacentHTML("afterbegin", full_div)

}

function msg_summary(messagesContainer, payload) {
    let full_div = `<div class="row console-callout console-callout-summary">
        <div class="col-md-1"><span class="glyphicon glyphicon-globe">&nbsp;</span></div>
        <div class="col-md-10">
            <h4>Sensor Summary</h4>
            <small> ${payload.hosts} sensor hosts</small>

            <h5>Sensor Types</h5>
            <pre>${JSON.stringify(payload.sensor_type, null, 4)}</pre>

            <h5>OSes</h5>
            <pre>${JSON.stringify(payload.sensor_os, null, 4)}</pre>
        </div>
    </div>`

  messagesContainer.insertAdjacentHTML("afterbegin", full_div)

}

function msg_info(messagesContainer, title, msg) {
    let full_div = `<div class="row console-callout console-callout-info"><div class="col-md-1"><span class="glyphicon glyphicon-info-sign">&nbsp;</span></div><div class="col-md-10"><h4>${title}</h4><p>${msg}</p></div></div>`;
    messagesContainer.insertAdjacentHTML("afterbegin", full_div)
}



function subscribe_to_c2() {


    // Now that you are connected, you can join channels with a topic:
    let channel = socket.channel("c2:all", {})

    let messagesContainer = document.querySelector("#messages")

    channel.on("c2_msg", payload => {

        if (payload.action == "heartbeat") {
            msg_heartbeat(messagesContainer, payload)
        }
        else if (payload.action == "sensor-registration") {
            msg_announce_sensor_reg(messagesContainer, payload)
        }
        else if (payload.action == "sensor-deregistration") {
            msg_announce_sensor_dereg(messagesContainer, payload)
        }
        else if (payload.action == "sensors-status") {
            msg_summary(messagesContainer, payload);
        }
        else if (payload.action == "sensor-observe") {
            msg_announce_sensor_observe(messagesContainer, payload);
        }
        else {
            console.log(payload)
        }
    })

    channel.join()
      .receive("ok", resp => { msg_info(messagesContainer, "Subscribed to C2", "Successfully subscribed to Sensing API Command and Control Monitoring stream.") })
      .receive("error", resp => { console.log("Unable to join", resp) })

}

export default {socket, subscribe_to_c2}
