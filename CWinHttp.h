#pragma once

#include <string>
#include <cstdio>
#include <vector>
#include <windows.h>

class CWinHttp {
public:
    /*
    url: リクエスト先のURL
    httpMethod: GET、POSTといったリクエスト方法を文字列で指定する
    headers: ヘッダー（各ヘッダーをvectorの要素として追加する）
    body: ボディ
    */
	struct CHTTP_DATA {
		std::wstring url;
		std::wstring httpMethod;
		std::vector< std::wstring > headers;
		std::string body;
	};

	bool executeWinHttp( const CHTTP_DATA &p_requestData, CHTTP_DATA &p_responseData, std::wstring &p_errorMessage );

private:
	bool receiveResponse( void *p_handle, CWinHttp::CHTTP_DATA &p_responseData, std::wstring &p_errorMessage );
	bool receiveHeader( void *p_handle, CWinHttp::CHTTP_DATA &p_responseData, std::wstring &p_errorMessage );
	bool receiveBody( void *p_handle, CWinHttp::CHTTP_DATA &p_responseData, std::wstring &p_errorMessage );
    void getLastErrorMessage( DWORD p_errorCode, const std::wstring &p_errorFunctionName, std::wstring &p_errorMessage );
};
