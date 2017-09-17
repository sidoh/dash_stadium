var FORM_SETTINGS = [
  "admin_username", "admin_password",
  "mqtt_server", "mqtt_topic_pattern", "mqtt_username", "mqtt_password",
  "ap_name", "ap_password",
  "debounce_threshold_ms"
];

var FORM_SETTINGS_HELP = {
  mqtt_server : "Domain or IP address of MQTT broker. Optionally specify a port " +
    "with (example) mymqqtbroker.com:1884.",
  mqtt_topic_pattern : "Pattern for MQTT topic. Example: " +
    "dash_stadium/:event_type/:mac_addr. See README for further details."
}

var webSocket = new WebSocket("ws://" + location.hostname + ":81");
webSocket.onmessage = function(e) {
  var data = JSON.parse(e.data);
  var line = "[" + (new Date()).toISOString() + "] event: "
    + data.event + ", mac: " + data.macAddr + "\n";
  $('.wifi-events').append(line);
}

var loadSettings = function() {
  $.getJSON('/settings', function(val) {
    Object.keys(val).forEach(function(k) {
      var field = $('#settings input[name="' + k + '"]');

      if (field.length > 0) {
        if (field.attr('type') === 'radio') {
          field.filter('[value="' + val[k] + '"]').click();
        } else {
          field.val(val[k]);
        }
      }
    });

    var deviceForm = $('#monitored-devices').html('');
    if (val.monitored_macs) {
      val.monitored_macs.forEach(function(v) {
        deviceForm.append(deviceRow(v[0], v[1]));
      });
    }
  });
};

var saveMonitoredDevices = function() {
  var form = $('#monitored-devices')
    , errors = false;

  $('input', form).removeClass('error');

  var macAddrs = $('input[name="macAddrs[]"]', form).map(function(i, v) {
    var val = $(v).val();

    if (!val.match(/^([a-f0-9]{1,2}:){5}[a-f0-9]{1,2}$/i)) {
      errors = true;
      $(v).addClass('error');
      return null;
    } else {
      return val;
    }
  });

  var deviceAliases = $('input[name="deviceAliases[]"]', form).map(function(i, v) {
    return $(v).val();
  });

  if (!errors) {
    var data = [];
    for (var i = 0; i < macAddrs.length; i++) {
      data[i] = [macAddrs[i], deviceAliases[i]];
    }
    $.ajax(
      '/settings',
      {
        method: 'put',
        contentType: 'application/json',
        data: JSON.stringify({monitored_macs: data})
      }
    )
  }
};

var deviceRow = function(macAddr, alias) {
  var elmt = '<tr>';
  elmt += '<td>';
  elmt += '<input name="macAddrs[]" class="form-control" value="' + macAddr + '"/>';
  elmt += '</td>';
  elmt += '<td>'
  elmt += '<input name="deviceAliases[]" class="form-control" value="' + alias + '"/>';;
  elmt += '</td>';
  elmt += '<td>';
  elmt += '<button class="btn btn-danger remove-device">';
  elmt += '<i class="glyphicon glyphicon-remove"></i>';
  elmt += '</button>';
  elmt += '</td>';
  elmt += '</tr>';
  return elmt;
}

$(function() {
  var settings = "";

  FORM_SETTINGS.forEach(function(k) {
    var elmt = '<div class="form-entry">';
    elmt += '<div>';
    elmt += '<label for="' + k + '">' + k + '</label>';

    if (FORM_SETTINGS_HELP[k]) {
      elmt += '<div class="field-help" data-help-text="' + FORM_SETTINGS_HELP[k] + '"></div>';
    }

    elmt += '</div>';
    elmt += '<input type="text" class="form-control" name="' + k + '"/>';
    elmt += '</div>';

    settings += elmt;
  });

  $('#settings').prepend(settings);
  $('#settings').submit(function(e) {
    var obj = {};

    FORM_SETTINGS.forEach(function(k) {
      var elmt = $('#settings input[name="' + k + '"]');

      if (elmt.attr('type') === 'radio') {
        obj[k] = elmt.filter(':checked').val();
      } else {
        obj[k] = elmt.val();
      }
    });

    $.ajax(
      "/settings",
      {
        method: 'put',
        contentType: 'application/json',
        data: JSON.stringify(obj)
      }
    );

    e.preventDefault();
    return false;
  });

  $('.field-help').each(function() {
    var elmt = $('<i></i>')
      .addClass('glyphicon glyphicon-question-sign')
      .tooltip({
        placement: 'top',
        title: $(this).data('help-text'),
        container: 'body'
      });
    $(this).append(elmt);
  });

  $('#add-device-btn').click(function() {
    $('#monitored-devices').append(deviceRow('', ''));
  });

  $('body').on('click', '.remove-device', function() {
    $(this).closest('tr').remove();
  });

  $('#monitored-devices-form').submit(function(e) {
    saveMonitoredDevices();
    e.preventDefault();
    return false;
  });

  loadSettings();
});
