if (this.document === undefined) {
  importScripts("/resources/testharness.js");
  importScripts("../resources/utils.js");
}

function checkContentType(contentType, body)
{
    if (self.FormData && body instanceof self.FormData) {
        assert_true(contentType.startsWith("multipart/form-data;boundary="), "Request should have header content-type starting with multipart/form-data;boundary=, but got " + contentType);
        return;
    }

    var expectedContentType = "text/plain;charset=UTF-8";
    if(body === null || body instanceof ArrayBuffer)
        expectedContentType = null;
    else if (body instanceof Blob)
        expectedContentType = body.type ? body.type : null;

    assert_equals(contentType , expectedContentType, "Request should have header content-type: " + expectedContentType);
}

function requestHeaders(desc, url, method, body, expectedOrigin, expectedContentLength) {
  var urlParameters = "?headers=origin|user-agent|accept-charset|content-length|content-type";
  var requestInit = {"method": method}
  if (body)
    requestInit["body"] = body;
  promise_test(function(test){
    return fetch(url + urlParameters, requestInit).then(function(resp) {
      assert_equals(resp.status, 200, "HTTP status is 200");
      assert_equals(resp.type , "basic", "Response's type is basic");
      checkContentType(resp.headers.get("x-request-content-type"), body);
      if (expectedContentLength !== undefined)
        assert_equals(resp.headers.get("x-request-content-length") , expectedContentLength, "Request should have header content-length: " + expectedContentLength);
      assert_true(resp.headers.has("x-request-user-agent"), "Request has header user-agent");
      assert_false(resp.headers.has("accept-charset"), "Request has header accept-charset");
      assert_equals(resp.headers.get("x-request-origin") , expectedOrigin, "Request should have header origin: " + expectedOrigin);
    });
  }, desc);
}

var url = RESOURCES_DIR + "inspect-headers.py"

requestHeaders("Fetch with GET", url, "GET", null, location.origin, null);
requestHeaders("Fetch with HEAD", url, "HEAD", null, location.origin, null);
requestHeaders("Fetch with PUT without body", url, "POST", null, location.origin, "0");
requestHeaders("Fetch with PUT with body", url, "PUT", "Request's body", location.origin, "14");
requestHeaders("Fetch with POST without body", url, "POST", null, location.origin, "0");
requestHeaders("Fetch with POST with text body", url, "POST", "Request's body", location.origin, "14");
if (self.FormData)
    requestHeaders("Fetch with POST with FormData body", url, "POST", new FormData(), location.origin);
requestHeaders("Fetch with POST with Blob body", url, "POST", new Blob(["Test"]), location.origin, "4");
requestHeaders("Fetch with POST with ArrayBuffer body", url, "POST", new ArrayBuffer(4), location.origin, "4");
requestHeaders("Fetch with POST with Blob body with mime type", url, "POST", new Blob(["Test"], { type: "text/maybe" }), location.origin, "4");
requestHeaders("Fetch with Chicken", url, "Chicken", null, location.origin, null);
requestHeaders("Fetch with Chicken with body", url, "Chicken", "Request's body", location.origin, "14");

done();
