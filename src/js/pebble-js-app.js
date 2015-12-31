const API_KEY = ''

var xhrRequest = function(url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    if (this.readyState == 4) {
      if (this.status == 200) {
        callback(this.responseText);
      } else {
        callback(false);
      }
    }
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  var apiKey = localStorage.getItem('forecastAPIKey');
  var apiEncoded = encodeURIComponent(apiKey.trim());

  var url = 'https://api.forecast.io/forecast/' + apiEncoded + '/' +
    pos.coords.latitude + ',' + pos.coords.longitude;

  // Send request to forecast.io.
  xhrRequest(url, 'GET',
      function(responseText) {
        if (responseText === false) {
          console.error('Bad response from server!');
        } else {
          // responseText contains a JSON object with weather info.
          var json = JSON.parse(responseText);

          var temperature = Math.round(json.currently.temperature);
          console.log('Temperature is: ' + temperature);

          var conditions = json.currently.summary;
          console.log('Conditions are: ' + conditions)

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
      }
  );
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  var apiKey = localStorage.getItem('forecastAPIKey');

  if (apiKey.trim().length == 0) {
    var dictionary = {
      'FORECAST_API_KEY': false
    };

    Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('Sent empty API key to pebble.');
        },
        function(e) {
          console.log('Error sending empty API key to pebble.');
        }
    );
  } else {
    navigator.geolocation.getCurrentPosition(
        locationSuccess,
        locationError,
        { timeout: 15000, maximumAge: 60000 }
    );
  }
}

// Listen for when the watchface is opened.
Pebble.addEventListener('ready',
    function(e) {
      console.log('PebbleKit JS ready!');

      // Get the initial weather
      getWeather();
    }
);

// Show the configuration page.
Pebble.addEventListener('showConfiguration', function(e) {
  var url = 'https://simpleface.watch';
  console.log('config url: ' + url);
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(configData));
});

// Listen for when an AppMessage is received.
Pebble.addEventListener('appmessage',
    function(e) {
      console.log('AppMessage received');
      getWeather();
    }
);
