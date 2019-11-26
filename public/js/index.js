// Trim whitespace
function trim(s) {
  return s.replace(/(^\s*)|(\s*$)/g, "");
}

/*async function fetchAudio(filename) {
  const requestOptions = {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({"filename": filename})
  };

  let url = "http://" + window.location.hostname + ":8080" + "/get";
  // fetch() returns a promise that resolved once headers have been received
  return fetch(url, requestOptions)
    .then(res => {
      if (!res.ok)
        throw new Error(`${res.status} = ${res.statusText}`);
      // response.body is a readable stream.
      // Calling getReader() gives us exclusive access to
      // the stream's content
      var reader = res.body.getReader();

      // read() returns a promise that resolves when a value has been received
      return reader
        .read()
        .then((result) => {
          return result;
        });
    })
}*/

function fetchAudio(filename, callback) {
  var url = "http://" + window.location.hostname + ":8080" + "/get";
  var xhr = new XMLHttpRequest();
  xhr.open('POST', url, true);
  xhr.setRequestHeader("Content-Type", "application/json");
  xhr.responseType = "arraybuffer";
  xhr.onload = function() {
    try {
      switch(this.status) {
        case 200:
          callback(xhr.response);
          break;
        case 404:
          throw 'File Not Found';
        default:
          throw 'Failed to fetch the file';
      }
    } catch(e) {
      console.error(e);
    }
  };
  var data = JSON.stringify({"filename": filename})
  xhr.send(data);
}

function getSomething() {
  var filename = document.getElementById('filename').value;
  var player = document.getElementById('player');
  if (trim(filename).length == 0) {
    alert("Please input the file name");
  } else {
    var mediaSource = new MediaSource();
    player.src = URL.createObjectURL(mediaSource);
    mediaSource.addEventListener('sourceopen', function() {
      var sourceBuffer = mediaSource.addSourceBuffer('audio/mpeg');

      fetchAudio(filename, buffer => {
        sourceBuffer.appendBuffer(buffer);
      });

      /*fetchAudio(filename)
        .then((response) => {
          sourceBuffer.appendBuffer(response.value);
        })
        .catch((error) => {
          this.setState({
            error: error.message
          });
        });*/
    });
  }
}
