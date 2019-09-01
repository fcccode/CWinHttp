# CWinHttp
  * Overview
    * C++ static library about WinHttp
  * Usage method
    1. Create the instance of CWinHttp and call executeWinHttp method
    2. The arguments of executeWinHttp include CHTTP_DATA structure which is defined in CWinHttp.h
       * url: Destination URL
       * httpMethod: The name of Http request methods like "GET", "POST" and so on
       * header: Http request header. In the current source, add each type of header to vector's element.
       * body: Http request body. In the current source, set cocanated body's value separated by "&" to this valuable 
         ```
         CWinHttp::CHTTP_DATA request;
         request.url = L"...";
         request.httpMethod = L"POST";
         request.header.emplace_back( L"Content-Type: ..." );
         request.body = "example_value=1&example_value2=2"
         ```
