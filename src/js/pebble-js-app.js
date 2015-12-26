const API_KEY = ''

var xhrRequest = function(url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  var url = 'https://api.forecast.io/forecast/' + API_KEY + '/' +
    pos.coords.latitude + ',' + pos.coords.longitude;

  // Send request to forecast.io.
  xhrRequest(url, 'GET',
      function(responseText) {
        // responseText contains a JSON object with weather info.
        var json = JSON.parse(responseText);

        var temperature = Math.round(json.currently.temperature);
        console.log('Temperature is: ' + temperature);

        var conditions = json.currently.summary;
        console.log('Conditions are: ' + conditions);

        // Assemble dictionary using our keys
        var dictionary = {
          'KEY_TEMPERATURE': temperature,
          'KEY_CONDITIONS': conditions
        };

        // Send to pebble.
        Pebble.sendAppMessage(dictionary,
            function(e) {
              console.log('Weather info sent to pebble successfully!');
            },
            function(e) {
              console.log('Error sending weather info to pebble!');
            }
        );
      }
  );
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
      locationSuccess,
      locationError,
      { timeout: 15000, maximumAge: 60000 }
  );
}

// Listen for when the watchface is opened.
Pebble.addEventListener('ready',
    function(e) {
      console.log('PebbleKit JS ready!');

      // Get the initial weather
      getWeather();
    }
);

// Listen for when an AppMessage is received.
Pebble.addEventListener('appmessage',
    function(e) {
      console.log('AppMessage received');
      getWeather();
    }
);
