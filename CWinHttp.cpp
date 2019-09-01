/*
CWinHttp

概要
  WinHttpによるHttp通信を行うクラス
*/
#include "CWinHttp.h"
#include <memory>
#include <winhttp.h>
#pragma comment( lib, "winhttp.lib" )

/*
executeWinHttp

概要
  WinHttpによるHttp通信を行う

引数
  p_requestData                                  リクエストデータ
  p_responseData                                 レスポンスデータ
  p_errorMessage                                 エラーメッセージ

戻り値
  bool                                           成功か失敗
*/
bool CWinHttp::executeWinHttp( const CWinHttp::CHTTP_DATA &p_requestData, CWinHttp::CHTTP_DATA &p_responseData, std::wstring &p_errorMessage ) {
	// 変数の宣言と初期化
	bool result = false;
	HINTERNET hSession = nullptr;
	HINTERNET hConnect = nullptr;
	HINTERNET hRequest = nullptr;

	// 初期化
	p_errorMessage = L"";

	do {
		// 実行環境でWinHttpがサポートされていないか
		if ( !WinHttpCheckPlatform() ) {
			this->getLastErrorMessage( ::GetLastError(), L"WinHttpCheckPlatform", p_errorMessage );
			break;
		}

		// URLのセット
		WCHAR szHostName[ 257 ];
		WCHAR szUrlPath[ 2049 ];
		URL_COMPONENTS urlComponents;
		::SecureZeroMemory( &urlComponents, sizeof( URL_COMPONENTS ) );
		urlComponents.dwStructSize = sizeof( URL_COMPONENTS );
		urlComponents.lpszHostName = szHostName;
		urlComponents.dwHostNameLength = sizeof( szHostName ) / sizeof( WCHAR );
		urlComponents.lpszUrlPath = szUrlPath;
		urlComponents.dwUrlPathLength = sizeof( szUrlPath ) / sizeof( WCHAR );

		// URLの解析に失敗したか
		if ( !WinHttpCrackUrl( p_requestData.url.c_str(), lstrlenW( p_requestData.url.c_str() ), ICU_ESCAPE, &urlComponents ) ) {
			this->getLastErrorMessage( ::GetLastError(), L"WinHttpCrackUrl", p_errorMessage );
			break;
		}

		// オープンに失敗したか
		hSession = WinHttpOpen( L"Agent", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );
		if ( hSession == nullptr ) {
			this->getLastErrorMessage( ::GetLastError(), L"WinHttpOpen", p_errorMessage );
			break;
		}

		// 接続に失敗したか
		hConnect = WinHttpConnect( hSession, urlComponents.lpszHostName, ( urlComponents.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT ), 0 );
		if ( hConnect == nullptr ) {
			this->getLastErrorMessage( ::GetLastError(), L"WinHttpConnect", p_errorMessage );
			break;
		}

		// リクエストのオープンに失敗したか
		hRequest = WinHttpOpenRequest( hConnect, p_requestData.httpMethod.c_str(), urlComponents.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, ( urlComponents.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0 ) );
		if ( hRequest == nullptr ) {
			this->getLastErrorMessage( ::GetLastError(), L"WinHttpOpenRequest", p_errorMessage );
			break;
		}

		// ヘッダーをセットする
		std::wstring header = L"";
		for ( size_t i = 0; i < p_requestData.headers.size(); i++ ) {
			if ( i > 0 ) {
				header += L"\r\n";
			}
			header += p_requestData.headers[ i ];
		}

		// ボディをセットしてリクエストを送信に失敗したか
		const char *body = p_requestData.body.c_str();
		DWORD bodyLength = ( DWORD )lstrlenA( body );
		if ( !WinHttpSendRequest( hRequest, header.c_str(), 0, ( LPVOID )body, bodyLength, bodyLength, 0 ) ) {
			this->getLastErrorMessage( ::GetLastError(), L"WinHttpSendRequest", p_errorMessage );
			break;
		}

		// レスポンスの受信に失敗したか
		if ( !this->receiveResponse( hRequest, p_responseData, p_errorMessage ) ) {
			break;
		}

		// 正常終了をセットする
		result = true;
	}
	while ( 0 );

	// ハンドルをそれぞれクローズする
	if ( hSession != nullptr ) WinHttpCloseHandle( hSession );
	if ( hConnect != nullptr ) WinHttpCloseHandle( hConnect );
	if ( hRequest != nullptr ) WinHttpCloseHandle( hRequest );

	// 戻り値を返す
	return result;
}

/*
receiveResponse

概要
  レスポンスの取得

引数
  p_handle                                       リクエストハンドル
  p_responseData                                 レスポンスデータ
  p_errorMessage                                 エラーメッセージ

戻り値
  bool                                           成功か失敗
*/
bool CWinHttp::receiveResponse( void *p_handle, CWinHttp::CHTTP_DATA &p_responseData, std::wstring &p_errorMessage ) {
	// 変数の宣言と初期化
	bool result = false;

    do {
        // レスポンスの受信に失敗したか
        if ( !WinHttpReceiveResponse( p_handle, nullptr ) ) {
            this->getLastErrorMessage( ::GetLastError(), L"WinHttpReceiveResponse", p_errorMessage );
            break;
        }

        // ヘッダーの受信に失敗したか
        if ( !this->receiveHeader( p_handle, p_responseData, p_errorMessage ) ) {
            break;
        }

        // ボディの受信に失敗したか
        if ( !this->receiveBody( p_handle, p_responseData, p_errorMessage ) ) {
            break;
        }

        result = true;
    }
    while ( 0 );

	// 戻り値を返す
	return result;
}

/*
receiveHeader

概要
  レスポンスヘッダーの取得

引数
  p_handle                                       リクエストハンドル
  p_responseData                                 レスポンスデータ
  p_errorMessage                                 エラーメッセージ

戻り値
  bool                                           成功か失敗
*/
bool CWinHttp::receiveHeader( void *p_handle, CWinHttp::CHTTP_DATA &p_responseData, std::wstring &p_errorMessage ) {
	// 変数の宣言と初期化
	bool result = false;
	DWORD dwBuffSize = 0;

	// 初期化
	p_responseData.headers.clear();

	// ヘッダーのバッファサイズを取得するために実行
	WinHttpQueryHeaders( p_handle, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwBuffSize, WINHTTP_NO_HEADER_INDEX );

	do {
		// ヘッダーのバッファサイズを取得するために実行し失敗したか
		std::unique_ptr< WCHAR[] > pwszHeader = nullptr;
		pwszHeader = std::make_unique< WCHAR[] >( ( dwBuffSize / sizeof( WCHAR ) ) + 1 );
		if ( !pwszHeader ) {
			this->getLastErrorMessage( ::GetLastError(), L"receiveHeader", p_errorMessage );
			break;
		}

		// ヘッダーの取得
		if ( !WinHttpQueryHeaders( p_handle, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, ( LPVOID )pwszHeader.get(), &dwBuffSize, WINHTTP_NO_HEADER_INDEX ) ) {
			this->getLastErrorMessage( ::GetLastError(), L"receiveHeader", p_errorMessage );
			break;
		}

		// ヘッダーを退避
		std::wstring resultHeader = pwszHeader.get();
		p_responseData.headers.emplace_back( resultHeader );

		// 成功のステータスをセット
		result = true;
	}
	while ( 0 );

	return result;
}

/*
receiveBody

概要
  レスポンスボディの取得

引数
  p_handle                                       リクエストハンドル
  p_responseData                                 レスポンスデータ
  p_errorMessage                                 エラーメッセージ

戻り値
  bool                                           成功か失敗
*/
bool CWinHttp::receiveBody( void *p_handle, CWinHttp::CHTTP_DATA &p_responseData, std::wstring &p_errorMessage ) {
	// 変数の宣言と初期化
	bool isError = false;

	for ( ;; ) {
		DWORD dwBuffSize = 0;
		DWORD dwDownloaded = 0;

		// 読み込み可能なボディを確認する際に失敗したか
		if ( !WinHttpQueryDataAvailable( p_handle, &dwBuffSize ) ) {
			isError = true;
			this->getLastErrorMessage( ::GetLastError(), L"receiveBody", p_errorMessage );
			break;
		}

		if ( dwBuffSize <= 0 ) {
			break;
		}

		// バッファの割り当て
		std::unique_ptr< char[] > pszOutBuffer = nullptr;
		pszOutBuffer = std::make_unique< char[] >( ( dwBuffSize / sizeof( char ) ) + 1 );
		if ( !pszOutBuffer ) {
			isError = true;
            this->getLastErrorMessage( ::GetLastError(), L"receiveBody", p_errorMessage );
			break;
		}

		// メモリのクリア
		::SecureZeroMemory( pszOutBuffer.get(), dwBuffSize );

		// ボディの取得に成功
		if ( !WinHttpReadData( p_handle, ( LPVOID )pszOutBuffer.get(), dwBuffSize, &dwDownloaded ) ) {
			isError = true;
			this->getLastErrorMessage( ::GetLastError(), L"receiveBody", p_errorMessage );
			break;
		}

		// ボディの退避
		std::string resultBody = pszOutBuffer.get();
		p_responseData.body += resultBody;
	}

	return ( !isError ? true : false );
}

/*
getLastErrorMessage

概要
  エラーメッセージの取得

引数
  p_errorCode                                    エラーコード(GetLastErrorで取得)
  p_errorFunctionName                            エラーが発生した関数名

戻り値
  std::wstring                                   エラーメッセージ
*/
void CWinHttp::getLastErrorMessage( DWORD p_errorCode, const std::wstring &p_errorFunctionName, std::wstring &p_errorMessage ) {
	// 変数の初期化
	LPVOID lpMsgBuf = nullptr;
	std::wstring errorCause = L"";
    p_errorMessage = L"";

	// エラーメッセージの取得
	::FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, GetModuleHandleW( L"winhttp.dll" ), p_errorCode, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), ( LPWSTR )& lpMsgBuf, 0, nullptr );

	// ポインタがNULLではない
	if ( lpMsgBuf != nullptr ) {
		// エラー原因を退避
		errorCause = ( LPWSTR )lpMsgBuf;

        // エラーメッセージの成型
        p_errorMessage = L"[ERROR : " + p_errorFunctionName + L"]" + errorCause;

		// 確保したメモリを解放する
		::LocalFree( lpMsgBuf );
	}
    else {
        p_errorMessage = L"[ERROR : " + p_errorFunctionName + L"]" + L"Failed to retrieve the error message";
    }
}
