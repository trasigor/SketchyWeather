var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  if (localStorage.getItem("var_sw_server")) {
    var i = (localStorage.getItem("var_sw_server") + '').indexOf("openweathermap", 0);
    if (i === -1 ? false : true) {
      openweathermap(pos);
    }
    else {
      getLocation(pos);
    }
  }
  else {
    getLocation(pos);
  }
}

function getLocation(pos) {
  if (localStorage.getItem("var_sw_custom_location")) {
      yahooweather(localStorage.getItem("var_sw_custom_location"), pos);
  }
  else {
    var url = 'http://nominatim.openstreetmap.org/reverse?format=json&lat=' + pos.coords.latitude + '&lon=' + pos.coords.longitude;
  
    // Send request to YahooWeather
    xhrRequest(url, 'GET', function(responseText) {
      if (typeof(responseText) == "undefined") {
        console.log(new Date() + " - No response from weather server. One more try.");
        openweathermap(pos);
      }
      else {
        // responseText contains a JSON object with weather info
        var json = JSON.parse(responseText);
        yahooweather(json.display_name, pos);
      }
    });
  }
}


function yahooweather(location, pos) {
  var temperature_format = "c";
  if (localStorage.getItem("var_sw_temperature_scale")) {
    var i = (localStorage.getItem("var_sw_temperature_scale") + '').indexOf("fahrenheit", 0);
    if (i === -1 ? false : true) {
      temperature_format = "f";
    }
  }
  
  // Construct URL
  var url = 'https://query.yahooapis.com/v1/public/yql?format=json&q=' +
            encodeURIComponent('select item.condition, item.forecast, atmosphere, item.lat, item.long, units, location from weather.forecast ') +
            encodeURIComponent('where woeid in (select woeid from geo.places(1) where text="' + location + '") and u="'+temperature_format+'"');
  
  // Send request to YahooWeather
  xhrRequest(url, 'GET', function(responseText) {
    if (typeof(responseText) == "undefined" || responseText.indexOf("failed to connect") >= 0) {
      console.log(new Date() + " - No response from weather server. One more try.");
      openweathermap(pos);
    }
    else {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);
      
      if (typeof json.query.results.channel === null || typeof json.query.results.channel !== 'object' || typeof json.query.results.channel[0] !== 'object' || 3200 == json.query.results.channel[0].item.condition.code) {
        console.log(new Date() + " - Incorrect response from weather server. One more try.");
        openweathermap(pos);
      }
      else {
        var conditions_id = 0;
        var temperature = "N/A";
        var humidity = "N/A";
        var tomorrow = "N/A";
        var after_tomorrow = "N/A";
        var tomorrow_conditions_id = "N/A";
        var after_tomorrow_conditions_id = "N/A";
        var info = Pebble.getActiveWatchInfo();
        var extraspace = '';
        if (0 === info.platform.localeCompare('emery')) {
          extraspace = "\n";
        }

        if (json.query.count>0) {
          var is_maxmin = false;
          if (localStorage.getItem("var_sw_forecast_min_max")) {
            var i = (localStorage.getItem("var_sw_forecast_min_max") + '').indexOf("maxmin", 0);
            if (i === -1 ? false : true) {
              is_maxmin = true;
            }
          }
          temperature = json.query.results.channel[0].item.condition.temp+"°"+
            json.query.results.channel[0].units.temperature;
          // Conditions
          conditions_id = yahooConditionToOpenWeatherMap(json.query.results.channel[0].item.condition.code);

          humidity = parseInt(json.query.results.channel[0].atmosphere.humidity);

          var days = ['Sun','Mon','Tue','Wed','Thu','Fri','Sat'];
          var dateCustom = new Date();
          var correction = 0;

          if (!json.query.results.channel[1].item.forecast.day.localeCompare(days[dateCustom.getDay()])) {
            correction++;
          }
          
          tomorrow = json.query.results.channel[1+correction].item.forecast.day+"\n"+"\n"+"\n"+extraspace;
          if (is_maxmin) {
            tomorrow += Math.round(json.query.results.channel[1+correction].item.forecast.high)+"°"+
                        json.query.results.channel[1+correction].units.temperature+"\n"+
                        Math.round(json.query.results.channel[1+correction].item.forecast.low)+"°"+
                        json.query.results.channel[1+correction].units.temperature;
          }
          else {
            tomorrow += Math.round(json.query.results.channel[1+correction].item.forecast.low)+"°"+
                        json.query.results.channel[1+correction].units.temperature+"\n"+
                        Math.round(json.query.results.channel[1+correction].item.forecast.high)+"°"+
                        json.query.results.channel[1+correction].units.temperature;
          }
          
          after_tomorrow = json.query.results.channel[2+correction].item.forecast.day+"\n"+"\n"+"\n"+extraspace;
          if (is_maxmin) {
            after_tomorrow += Math.round(json.query.results.channel[2+correction].item.forecast.high)+"°"+
                              json.query.results.channel[2+correction].units.temperature+"\n"+
                              Math.round(json.query.results.channel[2+correction].item.forecast.low)+"°"+
                              json.query.results.channel[2+correction].units.temperature;
          }
          else {
            after_tomorrow += Math.round(json.query.results.channel[2+correction].item.forecast.low)+"°"+
                              json.query.results.channel[2+correction].units.temperature+"\n"+
                              Math.round(json.query.results.channel[2+correction].item.forecast.high)+"°"+
                              json.query.results.channel[2+correction].units.temperature;
          }

          tomorrow_conditions_id = yahooConditionToOpenWeatherMap(json.query.results.channel[1+correction].item.forecast.code);

          after_tomorrow_conditions_id = yahooConditionToOpenWeatherMap(json.query.results.channel[2+correction].item.forecast.code);
        }

        // Assemble dictionary using our keys
        var dictionary = {
          "TEMPERATURE": temperature,
          "HUMIDITY": humidity,
          "CONDITIONS": conditions_id,
          "TOMORROW": tomorrow,
          "AFTER_TOMORROW": after_tomorrow,
          "TOMORROW_CONDITIONS": tomorrow_conditions_id,
          "AFTER_TOMORROW_CONDITIONS": after_tomorrow_conditions_id
        };

        // Send to Pebble
        Pebble.sendAppMessage(
          dictionary,
          function(e) {
            console.log("Weather info sent to Pebble successfully!");
          },
          function(e) {
            console.log("Error sending weather info to Pebble!");
          }
        );
      }
    }
  });
}

function openweathermap(pos) {
  var apikey = localStorage.getItem("var_sw_openweathermap_api_key");
  if (apikey && apikey != "undefined" && '' !== apikey) {
    openweathermapWithKey(pos, apikey);
  }
  else {
    var url = "http://pebble.itigor.com/sketchy-weather/tools.php?get_openweathermap_apikey";

    xhrRequest(url, 'GET', function(responseText) {
      var apikey = false;
      // responseText contains a JSON object
      var json = JSON.parse(responseText);

      if (json.apikey) {
        apikey = json.apikey;
      }

      openweathermapWithKey(pos, apikey);
    });
  }
}

function openweathermapWithKey(pos, apikey) {
  var temperature_format = "metric";
  if (localStorage.getItem("var_sw_temperature_scale")) {
    var i = (localStorage.getItem("var_sw_temperature_scale") + '').indexOf("fahrenheit", 0);
    if (i === -1 ? false : true) {
      temperature_format = "imperial";
    }
  }

  // Construct URL
  var urlCurrent = "",
      urlForecast = "",
      urlMain = "http://api.openweathermap.org/data/2.5/";
  if (localStorage.getItem("var_sw_custom_location")) {
    urlCurrent = urlMain+"weather?q=" + encodeURIComponent(localStorage.getItem("var_sw_custom_location"));
    urlForecast = urlMain+"forecast/daily?q=" + encodeURIComponent(localStorage.getItem("var_sw_custom_location"));
  }
  else {
    urlCurrent = urlMain+"weather?lat=" + pos.coords.latitude + "&lon=" + pos.coords.longitude;
    urlForecast = urlMain+"forecast/daily?lat=" + pos.coords.latitude + "&lon=" + pos.coords.longitude;
  }
  urlCurrent += '&units='+temperature_format;
  urlForecast += '&units='+temperature_format;

  if (apikey) {
    urlCurrent += '&APPID='+apikey;
    urlForecast += '&APPID='+apikey;
  }

  // Send request to OpenWeatherMap
  xhrRequest(urlCurrent, 'GET', function(responseText) {
    if (typeof(responseText) == "undefined" || responseText.indexOf("failed to connect") >= 0) {
      console.log(new Date() + " - No response from openweathermap weather server. One more try.");
      getLocation(pos);
    }
    else {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      var conditions_id = 0;
      var temperature = "N/A";
      var humidity = "N/A";

      if (parseInt(json.cod) != 404) {
        var scale = "°C";
        if (localStorage.getItem("var_sw_temperature_scale")) {
          var i = (localStorage.getItem("var_sw_temperature_scale") + '').indexOf("fahrenheit", 0);
          if (i === -1 ? false : true) {
            scale = "°F";
          }
        }

        temperature = Math.round(json.main.temp)+scale;
        humidity = Math.round(json.main.humidity);

        // Conditions
        conditions_id = getConditionsId(json.weather[0].icon, json.weather[0].id);

        // Send request to OpenWeatherMap to get forecast
        xhrRequest(urlForecast, 'GET', function(responseText) {
          if (typeof(responseText) == "undefined" || responseText.indexOf("failed to connect") >= 0) {
            console.log(new Date() + " - No forecast response from openweathermap weather server. One more try.");
            yahooweather(pos);
          }
          else {
            var info = Pebble.getActiveWatchInfo();
            var extraspace = '';
            if (0 === info.platform.localeCompare('emery')) {
              extraspace = "\n";
            }
            
            var is_maxmin = false;
            if (localStorage.getItem("var_sw_forecast_min_max")) {
              var i = (localStorage.getItem("var_sw_forecast_min_max") + '').indexOf("maxmin", 0);
              if (i === -1 ? false : true) {
                is_maxmin = true;
              }
            }
            // responseText contains a JSON object with weather info
            var json = JSON.parse(responseText);

            if (parseInt(json.cod) != 404) {
              var days = ['Sun','Mon','Tue','Wed','Thu','Fri','Sat'];
              var dateCustom = new Date();
              var today_day_of_week = dateCustom.getDay();
              var correction = 0;

              dateCustom.setTime(json.list[1].dt*1000);
              if (dateCustom.getDay() == today_day_of_week) {
                correction++;
                dateCustom.setTime(json.list[1+correction].dt*1000);
              }
              var tomorrow = days[dateCustom.getDay()]+"\n"+"\n"+"\n"+extraspace;
              if (is_maxmin) {
                  tomorrow += Math.round(json.list[1+correction].temp.max)+scale+"\n"+
                              Math.round(json.list[1+correction].temp.min)+scale;
              }
              else {
                  tomorrow += Math.round(json.list[1+correction].temp.min)+scale+"\n"+
                              Math.round(json.list[1+correction].temp.max)+scale;
              }

              var tomorrow_conditions_id = getConditionsId(json.list[1+correction].weather[0].icon, json.list[1+correction].weather[0].id);

              dateCustom.setTime(json.list[2+correction].dt*1000);
              var after_tomorrow =  days[dateCustom.getDay()]+"\n"+"\n"+"\n"+extraspace;
              if (is_maxmin) {
                after_tomorrow += Math.round(json.list[2+correction].temp.max)+scale+"\n"+
                                  Math.round(json.list[2+correction].temp.min)+scale;
              }
              else {
                after_tomorrow += Math.round(json.list[2+correction].temp.min)+scale+"\n"+
                                  Math.round(json.list[2+correction].temp.max)+scale;
              }
              var after_tomorrow_conditions_id = getConditionsId(json.list[2+correction].weather[0].icon, json.list[2+correction].weather[0].id);

              // Assemble dictionary using our keys
              var dictionary = {
                "TEMPERATURE": temperature,
                "HUMIDITY": humidity,
                "CONDITIONS": conditions_id,
                "TOMORROW": tomorrow,
                "AFTER_TOMORROW": after_tomorrow,
                "TOMORROW_CONDITIONS": tomorrow_conditions_id,
                "AFTER_TOMORROW_CONDITIONS": after_tomorrow_conditions_id
              };

              // Send to Pebble
              Pebble.sendAppMessage(
                dictionary,
                function(e) {
                  console.log("Weather info sent to Pebble successfully!");
                },
                function(e) {
                  console.log("Error sending weather info to Pebble!");
                }
              );
            }
            else {
              console.log(new Date() + " - Error forecast response from openweathermap weather server. One more try.");
              getLocation(pos);
            }
          }
        });
      }
      else {
        console.log(new Date() + " - Error response from openweathermap weather server. One more try.");
        getLocation(pos);
      }

    }
  });
}

function getConditionsId(icon, conditions_id) {
  if (!icon.localeCompare("01d") || conditions_id == 904) {
    conditions_id = 1010;
  }
  else if (!icon.localeCompare("01n")) {
    conditions_id = 1020;
  }
  else if (!icon.localeCompare("02d")) {
    conditions_id = 1110;
  }
  else if (!icon.localeCompare("02n")) {
    conditions_id = 1120;
  }
  
  return conditions_id;
}

function yahooConditionToOpenWeatherMap(code) {
  var returnCode = 0;
  if (code === 0) { //Tornado
    returnCode = 900;
  }
  else if (code == 1) { //tropical storm
    returnCode = 901;
  }
  else if (code == 2) { //Hurricane
    returnCode = 902;
  }
  else if (code == 3) { //severe thunderstorms
    returnCode = 212;
  }
  else if (code == 4) { //Thunderstorm
    returnCode = 211;
  }
  else if (code >= 5 && code <= 7) { //mixed rain and snow
    returnCode = 616;
  }
  else if (code >= 8 && code <= 9) { //Drizzle
    returnCode = 300;
  }
  else if ((code >= 10 && code <= 12) || code == 40) { //Rain
    returnCode = 313;
  }
  else if ((code >= 13 && code <= 16) || (code >= 41 && code <= 43) || code == 46) { //Snow
    returnCode = 601;
  }
  else if (code == 17) { //Hail
    returnCode = 906;
  }
  else if (code == 18) { //sleet
    returnCode = 611;
  }
  else if (code == 19) { //dust
    returnCode = 761;
  }
  else if (code == 20) { //foggy
    returnCode = 741;
  }
  else if (code == 21) { //haze
    returnCode = 721;
  }
  else if (code == 22) { //smoky
    returnCode = 711;
  }
  else if (code >= 23 && code <= 24) { //windy
    returnCode = 905;
  }
  else if (code == 25) { //cold
    returnCode = 903;
  }
  else if ((code >= 26 && code <= 28) || code == 44) { //cloudy
    returnCode = 802;
  }
  else if (code == 29) { //partly cloudy (night)
    returnCode = 1120;
  }
  else if (code == 30) { //partly cloudy (day)
    returnCode = 1110;
  }
  else if (code == 31 || code == 33) { //clear (night)
    returnCode = 1020;
  }
  else if (code == 32 || code == 34) { //clear (day)
    returnCode = 1010;
  }
  else if (code == 35) { //mixed rain and hail
    returnCode = 201;
  }
  else if (code == 36) { //hot
    returnCode = 904;
  }
  else if ((code >= 37 && code <= 39) || code == 45 || code == 47) { //thunderstorms
    returnCode = 201;
  }
  
  return returnCode;
}

function locationError(err) {
  console.log("Error requesting location!");
  console.warn('location error: ' + err.code + ' - ' + err.message);
}

function getWeather() {
  if (localStorage.getItem("var_sw_custom_location")) {
    locationSuccess(false);
  }
  else {
    navigator.geolocation.getCurrentPosition(
      locationSuccess,
      locationError,
      {timeout: 15000, maximumAge: 2100000}
    );
  }
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', function(e) {
  console.log("PebbleKit JS ready!");

  // Get the initial weather
  getWeather();
});

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage', function(e) {
  console.log("AppMessage received!");
  getWeather();
});

Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  var location = localStorage.getItem("var_sw_custom_location") ? localStorage.getItem("var_sw_custom_location") : "";
  var temperature_scale = localStorage.getItem("var_sw_temperature_scale") ? localStorage.getItem("var_sw_temperature_scale") : "";
  var bluetooth = localStorage.getItem("var_sw_bluetooth") ? localStorage.getItem("var_sw_bluetooth") : "";
  var battery = localStorage.getItem("var_sw_battery") ? localStorage.getItem("var_sw_battery") : "";
  var server = localStorage.getItem("var_sw_server") ? localStorage.getItem("var_sw_server") : "";
  var screen_color = localStorage.getItem("var_sw_screen_color") ? localStorage.getItem("var_sw_screen_color") : "";
  var hourly_beep = localStorage.getItem("var_sw_hourly_beep") ? localStorage.getItem("var_sw_hourly_beep") : "";
  var dnd = localStorage.getItem("var_sw_dnd") ? localStorage.getItem("var_sw_dnd") : "";
  var dnd_start = localStorage.getItem("var_sw_dnd_start") ? localStorage.getItem("var_sw_dnd_start") : "";
  var dnd_end = localStorage.getItem("var_sw_dnd_end") ? localStorage.getItem("var_sw_dnd_end") : "";
  var hour_lead_zero = localStorage.getItem("var_sw_hour_lead_zero") ? localStorage.getItem("var_sw_hour_lead_zero") : "";
  var forecast_show_duration = localStorage.getItem("var_sw_forecast_show_duration") ? localStorage.getItem("var_sw_forecast_show_duration") : "";
  var main_screen_info = localStorage.getItem("var_sw_main_screen_info") ? localStorage.getItem("var_sw_main_screen_info") : "";
  var forecast_min_max = localStorage.getItem("var_sw_forecast_min_max") ? localStorage.getItem("var_sw_forecast_min_max") : "";
  var forecast_shake = localStorage.getItem("var_sw_forecast_shake") ? localStorage.getItem("var_sw_forecast_shake") : "";
  var openweathermap_api_key = localStorage.getItem("var_sw_openweathermap_api_key") ? localStorage.getItem("var_sw_openweathermap_api_key") : "";

  Pebble.openURL('http://pebble.itigor.com/sketchy-weather/configurations.php?version=6.3' + //appinfo.versionCode +
                 '&location=' + encodeURIComponent(location) +
                 '&temperature_scale=' + encodeURIComponent(temperature_scale) +
                 '&bluetooth=' + encodeURIComponent(bluetooth) +
                 '&battery=' + encodeURIComponent(battery) +
                 '&server=' + encodeURIComponent(server) +
                 '&screen_color=' + encodeURIComponent(screen_color) +
                 '&hourly_beep=' + encodeURIComponent(hourly_beep) +
                 '&dnd=' + encodeURIComponent(dnd) +
                 '&dnd_start=' + encodeURIComponent(dnd_start) +
                 '&dnd_end=' + encodeURIComponent(dnd_end) +
                 '&hour_lead_zero=' + encodeURIComponent(hour_lead_zero) +
                 '&forecast_show_duration=' + encodeURIComponent(forecast_show_duration) +
                 '&main_screen_info=' + encodeURIComponent(main_screen_info) +
                 '&forecast_min_max=' + encodeURIComponent(forecast_min_max) +
                 '&forecast_shake=' + encodeURIComponent(forecast_shake) +
                 '&openweathermap_api_key=' + encodeURIComponent(openweathermap_api_key));
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e.response) {
    var configurations = JSON.parse(decodeURIComponent(e.response));

    localStorage.setItem("var_sw_custom_location", configurations.location);
    localStorage.setItem("var_sw_temperature_scale", configurations.temperature_scale);
    localStorage.setItem("var_sw_bluetooth", configurations.bluetooth);
    localStorage.setItem("var_sw_battery", configurations.battery);
    localStorage.setItem("var_sw_server", configurations.server);
    localStorage.setItem("var_sw_screen_color", configurations.screen_color);
    localStorage.setItem("var_sw_hourly_beep", configurations.hourly_beep);
    localStorage.setItem("var_sw_dnd", configurations.dnd);
    localStorage.setItem("var_sw_dnd_start", configurations.dnd_start);
    localStorage.setItem("var_sw_dnd_end", configurations.dnd_end);
    localStorage.setItem("var_sw_hour_lead_zero", configurations.hour_lead_zero);
    localStorage.setItem("var_sw_forecast_show_duration", configurations.forecast_show_duration);
    localStorage.setItem("var_sw_main_screen_info", configurations.main_screen_info);
    localStorage.setItem("var_sw_forecast_min_max", configurations.forecast_min_max);
    localStorage.setItem("var_sw_forecast_shake", configurations.forecast_shake);
    localStorage.setItem("var_sw_openweathermap_api_key", configurations.openweathermap_api_key);

    var forecast_show_duration = parseInt(configurations.forecast_show_duration);
    forecast_show_duration = 'NaN'.localeCompare(forecast_show_duration) ? forecast_show_duration : 0;

    var settings = {
      "BLUETOOTH"              : configurations.bluetooth,
      "BATTERY"                : (configurations.battery.localeCompare("battery_always") === 0) ? true : false,
      "SCREEN_COLOR"           : configurations.screen_color,
      "HOURLY_BEEP"            : (configurations.hourly_beep.localeCompare("beep") === 0) ? true : false,
      "DND"                    : (configurations.dnd.localeCompare("dnd_on") === 0) ? true : false,
      "HOUR_LEAD_ZERO"         : (configurations.hour_lead_zero.localeCompare("show_zero_in_hours") === 0) ? true : false,
      "FORECAST_DURATION"      : forecast_show_duration,
      "MAIN_SCREEN_INFO"       : configurations.main_screen_info,
      "FORECAST_ON_ONE_SHAKE"  : (configurations.forecast_shake.localeCompare("one_shake") === 0) ? true : false
    };

    var dnd_start = parseInt(configurations.dnd_start);
    var dnd_end = parseInt(configurations.dnd_end);
    if (!configurations.dnd.localeCompare("dnd_on")) {
      if (isNaN(dnd_start) || isNaN(dnd_end)) {
        dnd_start = 0;
        dnd_end = 0;
      }
      settings.DND_START = dnd_start;
      settings.DND_END = dnd_end;
    }

    Pebble.sendAppMessage(
      settings,
      function(e) {
        console.log("Configurations sent to Pebble successfully!");
      },
      function(e) {
        console.log("Error sending configurations to Pebble!");
      }
    );
  }
  else {
    console.log('Configuration canceled');
  }
});