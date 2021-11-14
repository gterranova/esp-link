//===== alexa cards
function changeDevices(e) {
  e.preventDefault();
  var url = "alexa?1=1";
  var i, inputs = document.querySelectorAll('#devicesform input');
  for (i = 0; i < inputs.length; i++) {
    if (inputs[i].type != "checkbox" && inputs[i].type != "radio") 
        url += "&" + inputs[i].name + "=" + inputs[i].value;
    else if (inputs[i].type == "checkbox") 
        url += "&" + inputs[i].name + "=" + (inputs[i].checked?"1":"0");
  };

  hideWarning();
  var cb = $("#set-devices");
  addClass(cb, 'pure-button-disabled');
  ajaxSpin("POST", url, function (resp) {
    showNotification("Devices updated");
    removeClass(cb, 'pure-button-disabled');
    window.setTimeout(fetchAlexa, 100);
  }, function (s, st) {
    showWarning("Error: " + st);
    removeClass(cb, 'pure-button-disabled');
    window.setTimeout(fetchAlexa, 100);
  });
}
function changeAlexa(e) {
  e.preventDefault();
  var url = "alexa?1=1";
  var i, inputs = document.querySelectorAll('#alexa-form input');
  for (i = 0; i < inputs.length; i++) {
    if (inputs[i].type != "checkbox" && inputs[i].type != "radio") 
        url += "&" + inputs[i].name + "=" + inputs[i].value;
  };
  url += "&wemo-emulation=" + document.querySelector('input[name="wemo-emulation"]:checked').value;

  hideWarning();
  var cb = $("#alexa-button");
  addClass(cb, 'pure-button-disabled');
  ajaxSpin("POST", url, function (resp) {
    showNotification("Alexa updated");
    removeClass(cb, 'pure-button-disabled');
    window.setTimeout(fetchAlexa, 100);
  }, function (s, st) {
    showWarning("Error: " + st);
    removeClass(cb, 'pure-button-disabled');
    window.setTimeout(fetchAlexa, 100);
  });
}

function setupDevices(data) {
  while ($(".device").length) domForEach($(".device"), function (div) { div.remove(); });
  var l = $("#devicesform");
  var o = l.childNodes[0];
  // spinner
  var num = data['alexa-devices'];

  var i;
  for (i=0; i<num; i++) {
    l.insertBefore(m('<div class="device"><label>Device '+(i+1)+'</label><div><input type="checkbox" name="d'+i+'-state"/><label>State</label></div><input type="range" min="1" max="254" name="d'+i+'-bri"><label>Bri</label><hr/></div>'), o);
    document.querySelector('input[name="d'+i+'-bri"]').oninput = function () { this.nextElementSibling.innerHTML = "Bri ("+this.value+")"; };
  }
}

function displayAlexa(data) {
  setupDevices(data);
  Object.keys(data).forEach(function (v) {
    el = $("#" + v);
    if (el != null) {
      if (el.nodeName === "INPUT") el.value = data[v];
      else el.innerHTML = data[v];
      return;
    }
    el = document.querySelector('input[name="' + v + '"]');
    if (el != null) {
      if (el.type == "checkbox") el.checked = data[v] > 0;
      else if (el.type != "radio") el.value = data[v];
    }
    el = $("#" + v+"-ron");
    if (el != null) {
      if (el.type === "radio") el.checked = data[v];
    }
    el = $("#" + v+"-roff");
    if (el != null) {
      if (el.type === "radio") el.checked = !data[v];
    }
  });

  $("#spinner").setAttribute("hidden", "");
  $("#devices-spinner").setAttribute("hidden", "");
  $("#alexa-form").removeAttribute("hidden");

  var i, inputs = document.querySelectorAll('#alexa-form input');
  for (i = 0; i < inputs.length; i++) {
    if (inputs[i].type == "checkbox")
      inputs[i].onclick = function () { setAlexa(this.name, this.checked) };
  }
}

function fetchAlexa() {
  ajaxJsonSpin("GET", "/alexa", displayAlexa, function () {
    window.setTimeout(fetchAlexa, 1000);
  });
}

function setAlexa(name, v) {
  ajaxSpin("POST", "/alexa?" + name + "=" + (v ? 1 : 0), function () {
    var n = name.replace("-enable", "");
    showNotification(n + " is now " + (v ? "enabled" : "disabled"));
  }, function () {
    showWarning("Enable/disable failed");
    window.setTimeout(fetchAlexa, 100);
  });
}

function doWemo() {
  $('#wemo-emulation-ron').checked = true;
  $('#wemo-emulation-roff').checked = false;
  //$('#dhcp-off').setAttribute('hidden', '');
}

function doHue() {
  $('#wemo-emulation-roff').checked = true;
  $('#wemo-emulation-ron').checked = false;
  //$('#dhcp-off').removeAttribute('hidden');
  //$('#dhcp-on').setAttribute('hidden', '');
}
