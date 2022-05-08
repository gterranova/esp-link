//===== spiffs cards
function listFiles(data) {
  while ($(".file").length) domForEach($(".file"), function (div) { div.remove(); });
  var l = $("#filesform");
  var o = l.childNodes[0];
  // spinner
  var num = data['files'].length;

  var i;
  for (i=0; i<num; i++) {
    let f = data['files'][i];
    l.insertBefore(m('<div class="file"><a href="/spiffs'+f.name+'">'+f.name+'</label></div>'), o);
  }
}

function displaySpiffs(data) {
  listFiles(data);
  $("#spinner").setAttribute("hidden", "");
  $("#files-spinner").setAttribute("hidden", "");
  $("#spiffs-form").removeAttribute("hidden");
}

function fetchSpiffs() {
  ajaxJsonSpin("GET", "/spiffs", displaySpiffs, function () {
    window.setTimeout(fetchSpiffs, 1000);
  });
}
