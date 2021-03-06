/*
 * HttpRequest.cpp
 *
 *  Created on: Aug 9, 2012
 *      Author: lion
 */

#include "HttpRequest.h"
#include "parse.h"
#include "HttpUploadCollection.h"
#include "Url.h"

namespace fibjs
{

result_t HttpRequest_base::_new(obj_ptr<HttpRequest_base> &retVal, v8::Local<v8::Object> This)
{
    retVal = new HttpRequest();
    return 0;
}

result_t HttpRequest::get_protocol(qstring &retVal)
{
    return m_message->get_protocol(retVal);
}

result_t HttpRequest::set_protocol(const char *newVal)
{
    return m_message->set_protocol(newVal);
}

result_t HttpRequest::get_headers(obj_ptr<HttpCollection_base> &retVal)
{
    return m_message->get_headers(retVal);
}

result_t HttpRequest::get_body(obj_ptr<SeekableStream_base> &retVal)
{
    return m_message->get_body(retVal);
}

result_t HttpRequest::set_body(SeekableStream_base *newVal)
{
    return m_message->set_body(newVal);
}

result_t HttpRequest::read(int32_t bytes, obj_ptr<Buffer_base> &retVal,
                           AsyncEvent *ac)
{
    return m_message->read(bytes, retVal, ac);
}

result_t HttpRequest::readAll(obj_ptr<Buffer_base> &retVal, AsyncEvent *ac)
{
    return m_message->readAll(retVal, ac);
}

result_t HttpRequest::write(Buffer_base *data, AsyncEvent *ac)
{
    return m_message->write(data, ac);
}

result_t HttpRequest::get_length(int64_t &retVal)
{
    return m_message->get_length(retVal);
}

result_t HttpRequest::get_keepAlive(bool &retVal)
{
    return m_message->get_keepAlive(retVal);
}

result_t HttpRequest::set_keepAlive(bool newVal)
{
    return m_message->set_keepAlive(newVal);
}

result_t HttpRequest::get_upgrade(bool &retVal)
{
    return m_message->get_upgrade(retVal);
}

result_t HttpRequest::set_upgrade(bool newVal)
{
    return m_message->set_upgrade(newVal);
}

result_t HttpRequest::get_maxHeadersCount(int32_t &retVal)
{
    return m_message->get_maxHeadersCount(retVal);
}

result_t HttpRequest::set_maxHeadersCount(int32_t newVal)
{
    return m_message->set_maxHeadersCount(newVal);
}

result_t HttpRequest::get_maxUploadSize(int32_t &retVal)
{
    return m_message->get_maxUploadSize(retVal);
}

result_t HttpRequest::set_maxUploadSize(int32_t newVal)
{
    return m_message->get_maxUploadSize(newVal);
}

result_t HttpRequest::hasHeader(const char *name, bool &retVal)
{
    return m_message->hasHeader(name, retVal);
}

result_t HttpRequest::firstHeader(const char *name, Variant &retVal)
{
    return m_message->firstHeader(name, retVal);
}

result_t HttpRequest::allHeader(const char *name, obj_ptr<List_base> &retVal)
{
    return m_message->allHeader(name, retVal);
}

result_t HttpRequest::addHeader(Map_base* map)
{
    return m_message->addHeader(map);
}

result_t HttpRequest::addHeader(const char *name, Variant value)
{
    return m_message->addHeader(name, value);
}

result_t HttpRequest::setHeader(Map_base* map)
{
    return m_message->setHeader(map);
}

result_t HttpRequest::setHeader(const char *name, Variant value)
{
    return m_message->setHeader(name, value);
}

result_t HttpRequest::removeHeader(const char *name)
{
    return m_message->removeHeader(name);
}

result_t HttpRequest::get_value(qstring &retVal)
{
    return m_message->get_value(retVal);
}

result_t HttpRequest::set_value(const char *newVal)
{
    return m_message->set_value(newVal);
}

result_t HttpRequest::get_params(obj_ptr<List_base> &retVal)
{
    return m_message->get_params(retVal);
}

result_t HttpRequest::set_params(List_base *newVal)
{
    return m_message->set_params(newVal);
}

result_t HttpRequest::get_result(Variant &retVal)
{
    return m_message->get_result(retVal);
}

result_t HttpRequest::set_result(Variant newVal)
{
    return m_message->set_result(newVal);
}

result_t HttpRequest::get_lastError(qstring& retVal)
{
    return m_message->get_lastError(retVal);
}

result_t HttpRequest::set_lastError(const char* newVal)
{
    return m_message->set_lastError(newVal);
}

result_t HttpRequest::clear()
{
    m_message = new HttpMessage();

    m_method.assign("GET", 3);
    m_address.assign("/", 1);
    m_queryString.clear();

    if (m_response)
        m_response->clear();

    m_cookies.Release();
    m_query.Release();
    m_form.Release();

    return 0;
}

result_t HttpRequest::sendTo(Stream_base *stm, AsyncEvent *ac)
{
    if (!ac)
        return CHECK_ERROR(CALL_E_NOSYNC);

    qstring strCommand = m_method;
    qstring strProtocol;

    strCommand.append(1, ' ');
    strCommand.append(m_address);
    if (!m_queryString.empty())
    {
        strCommand.append(1, '?');
        strCommand.append(m_queryString);
    }

    get_protocol(strProtocol);
    strCommand.append(1, ' ');
    strCommand.append(strProtocol);

    return m_message->sendTo(stm, strCommand, ac);
}

result_t HttpRequest::readFrom(Stream_base *stm, AsyncEvent *ac)
{
    class asyncReadFrom: public AsyncState
    {
    public:
        asyncReadFrom(HttpRequest *pThis, BufferedStream_base *stm,
                      AsyncEvent *ac) :
            AsyncState(ac), m_pThis(pThis), m_stm(stm)
        {
            m_pThis->clear();
            set(begin);
        }

        static int32_t begin(AsyncState *pState, int32_t n)
        {
            asyncReadFrom *pThis = (asyncReadFrom *) pState;

            pThis->set(command);
            return pThis->m_stm->readLine(HTTP_MAX_LINE, pThis->m_strLine,
                                          pThis);
        }

        static int32_t command(AsyncState *pState, int32_t n)
        {
            asyncReadFrom *pThis = (asyncReadFrom *) pState;

            if (n == CALL_RETURN_NULL)
                return pThis->done(CALL_RETURN_NULL);

            _parser p(pThis->m_strLine);
            result_t hr;

            if (!p.getWord(pThis->m_pThis->m_method))
                return CHECK_ERROR(Runtime::setError("HttpRequest: bad method."));

            p.skipSpace();

            qstring &addr = pThis->m_pThis->m_address;

            if (!p.getWord(addr, '?'))
                return CHECK_ERROR(Runtime::setError("HttpRequest: bad address."));

            if (!qstricmp(addr.c_str(), "http://", 7))
            {
                const char *p = qstrchr(addr.c_str() + 7, '/');
                if (p)
                    pThis->m_pThis->m_message->set_value(p);
            }
            else
                pThis->m_pThis->m_message->set_value(addr);

            if (p.want('?'))
                p.getWord(pThis->m_pThis->m_queryString);

            p.skipSpace();

            if (p.end())
                return CHECK_ERROR(Runtime::setError("HttpRequest: bad protocol version."));

            hr = pThis->m_pThis->set_protocol(p.now());
            if (hr < 0)
                return hr;

            pThis->done();
            return pThis->m_pThis->m_message->readFrom(pThis->m_stm, pThis);
        }

    public:
        obj_ptr<HttpRequest> m_pThis;
        obj_ptr<BufferedStream_base> m_stm;
        qstring m_strLine;
    };

    if (!ac)
        return CHECK_ERROR(CALL_E_NOSYNC);

    obj_ptr<BufferedStream_base> _stm = BufferedStream_base::getInstance(stm);
    if (!_stm)
        return CHECK_ERROR(Runtime::setError("HttpRequest: only accept BufferedStream object."));

    return (new asyncReadFrom(this, _stm, ac))->post(0);
}

result_t HttpRequest::get_method(qstring &retVal)
{
    retVal = m_method;
    return 0;
}

result_t HttpRequest::get_stream(obj_ptr<Stream_base> &retVal)
{
    return m_message->get_stream(retVal);
}

result_t HttpRequest::set_method(const char *newVal)
{
    m_method = newVal;
    return 0;
}

result_t HttpRequest::get_address(qstring &retVal)
{
    retVal = m_address;
    return 0;
}

result_t HttpRequest::set_address(const char *newVal)
{
    m_address = newVal;
    return 0;
}

result_t HttpRequest::get_queryString(qstring &retVal)
{
    retVal = m_queryString;
    return 0;
}

result_t HttpRequest::set_queryString(const char *newVal)
{
    m_queryString = newVal;
    return 0;
}

result_t HttpRequest::get_response(obj_ptr<Message_base> &retVal)
{
    if (!m_response)
        m_response = new HttpResponse();

    retVal = m_response;
    return 0;
}

void HttpRequest::parse(qstring &str, char split,
                        obj_ptr<HttpCollection_base> &retVal)
{
    obj_ptr<HttpCollection> c = new HttpCollection();

    const char *pstr = str.c_str();
    int32_t nSize = (int32_t) str.length();
    const char *pstrTemp;
    qstring strKey, strValue;

    while (nSize)
    {
        while (nSize && *pstr == ' ')
        {
            pstr++;
            nSize--;
        }

        pstrTemp = pstr;
        while (nSize && *pstr != '=' && *pstr != split)
        {
            pstr++;
            nSize--;
        }

        if (pstr > pstrTemp)
            Url::decodeURI(pstrTemp, (int32_t) (pstr - pstrTemp), strKey, true);
        else
            strKey.clear();

        if (nSize && *pstr == '=')
        {
            nSize--;
            pstr++;
        }

        pstrTemp = pstr;
        while (nSize && *pstr != split)
        {
            pstr++;
            nSize--;
        }

        if (!strKey.empty())
        {
            if (pstr > pstrTemp)
                Url::decodeURI(pstrTemp, (int32_t) (pstr - pstrTemp), strValue, true);
            else
                strValue.clear();
        }

        if (nSize)
        {
            nSize--;
            pstr++;
        }

        if (!strKey.empty())
            c->add(strKey, strValue);
    }

    retVal = c;
}

result_t HttpRequest::get_cookies(obj_ptr<HttpCollection_base> &retVal)
{
    if (!m_cookies)
    {
        qstring strCookie;

        header("cookie", strCookie);
        parse(strCookie, ';', m_cookies);
    }

    retVal = m_cookies;
    return 0;
}

result_t HttpRequest::get_form(obj_ptr<HttpCollection_base> &retVal)
{
    if (m_form == NULL)
    {
        int64_t len = 0;

        get_length(len);
        if (len == 0)
            m_form = new HttpCollection();
        else
        {
            qstring strType;
            bool bUpload = false;
            Variant v;

            if (firstHeader("Content-Type", v) == CALL_RETURN_NULL)
                return CHECK_ERROR(Runtime::setError("HttpRequest: Content-Type is missing."));

            strType = v.string();

            if (!qstricmp(strType.c_str(), "multipart/form-data;", 20))
                bUpload = true;
            else if (qstricmp(strType.c_str(),
                              "application/x-www-form-urlencoded", 33))
                return CHECK_ERROR(Runtime::setError("HttpRequest: unknown form format: " + strType));

            obj_ptr<Buffer_base> buf;
            obj_ptr<SeekableStream_base> _body;

            get_body (_body);
            _body->rewind();
            result_t hr = _body->read((int32_t) len, buf, NULL);
            if (hr < 0)
                return hr;

            qstring strForm;
            buf->toString(strForm);

            if (bUpload)
            {
                obj_ptr<HttpUploadCollection> col =
                    new HttpUploadCollection();
                col->parse(strForm, strType.c_str());
                m_form = col;
            }
            else
                parse(strForm, '&', m_form);
        }
    }

    retVal = m_form;

    return 0;
}

result_t HttpRequest::get_query(obj_ptr<HttpCollection_base> &retVal)
{
    if (m_query == NULL)
        parse(m_queryString, '&', m_query);

    retVal = m_query;
    return 0;
}

} /* namespace fibjs */
