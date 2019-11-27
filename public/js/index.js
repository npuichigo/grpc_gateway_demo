// Trim whitespace
function trim(s) {
  return s.replace(/(^\s*)|(\s*$)/g, "");
}

function getSomething() {
  var filename = document.getElementById('filename').value;
  if (trim(filename).length == 0) {
    alert("Please input the file name");
  } else {
    var url = "http://" + window.location.hostname + ":8080" + "/get?";
    url += encodeURI("filename=" + filename)
    var player = document.getElementById('player');
    player.src = url;
    player.play();
  }
}
