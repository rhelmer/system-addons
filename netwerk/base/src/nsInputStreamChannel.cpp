/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsInputStreamChannel.h"
#include "nsCOMPtr.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsIMIMEService.h"
#include "nsIFileTransportService.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kMIMEServiceCID, NS_MIMESERVICE_CID);
static NS_DEFINE_CID(kFileTransportServiceCID, NS_FILETRANSPORTSERVICE_CID);

////////////////////////////////////////////////////////////////////////////////
// nsInputStreamChannel methods:

nsInputStreamChannel::nsInputStreamChannel()
    : mContentType(nsnull)
{
    NS_INIT_REFCNT(); 
}

nsInputStreamChannel::~nsInputStreamChannel()
{
    if (mContentType) nsCRT::free(mContentType);
}

NS_METHOD
nsInputStreamChannel::Create(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult)
{
    nsInputStreamChannel* about = new nsInputStreamChannel();
    if (about == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(about);
    nsresult rv = about->QueryInterface(aIID, aResult);
    NS_RELEASE(about);
    return rv;
}

nsresult
nsInputStreamChannel::Init(nsIURI* uri, const char* contentType,
                           nsIInputStream* in, nsILoadGroup* group)
{
    mURI = uri;
    mLoadGroup = group;
    nsCAutoString cType(contentType);
    cType.ToLowerCase();
    mContentType = cType.ToNewCString();
    if (mContentType == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    mInputStream = in;
    return NS_OK;
}

NS_IMPL_ISUPPORTS4(nsInputStreamChannel, 
                   nsIChannel,
                   nsIRequest,
                   nsIStreamObserver,
                   nsIStreamListener);

////////////////////////////////////////////////////////////////////////////////
// nsIRequest methods:

NS_IMETHODIMP
nsInputStreamChannel::IsPending(PRBool *result)
{
    if (mFileTransport)
        return mFileTransport->IsPending(result);
    *result = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::Cancel(void)
{
    if (mFileTransport)
        return mFileTransport->Cancel();
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::Suspend(void)
{
    if (mFileTransport)
        return mFileTransport->Suspend();
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::Resume(void)
{
    if (mFileTransport)
        return mFileTransport->Resume();
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsIChannel methods:

NS_IMETHODIMP
nsInputStreamChannel::GetURI(nsIURI * *aURI)
{
    *aURI = mURI;
    NS_IF_ADDREF(*aURI);
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::OpenInputStream(PRUint32 startPosition, PRInt32 readCount,
                                      nsIInputStream **result)
{
    // if we had seekable streams, we could seek here:
    NS_ASSERTION(startPosition == 0, "Can't seek in nsInputStreamChannel");
    *result = mInputStream;
    NS_ADDREF(*result);
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::OpenOutputStream(PRUint32 startPosition, nsIOutputStream **_retval)
{
    // we don't do output
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsInputStreamChannel::AsyncRead(PRUint32 startPosition, PRInt32 readCount,
                                nsISupports *ctxt, nsIStreamListener *listener)
{
#if 0
    // currently this happens before AsyncRead returns -- hope that's ok
    nsresult rv;

    // Do an extra AddRef so that this method's synchronous operation doesn't end up destroying
    // the listener prematurely.
    nsCOMPtr<nsIStreamListener> l(listener);

    rv = listener->OnStartRequest(this, ctxt);
    if (NS_FAILED(rv)) return rv;

    PRUint32 amt;
    while (PR_TRUE) {
        rv = mInputStream->Available(&amt);
        if (NS_FAILED(rv)) break;
        if (amt == 0) 
            break;
        if (readCount != -1)
            amt = PR_MIN((PRUint32)readCount, amt);
        rv = listener->OnDataAvailable(this, ctxt, mInputStream, 0, amt);
        if (NS_FAILED(rv)) break;
    }

    rv = listener->OnStopRequest(this, ctxt, rv, nsnull);       // XXX error message 
    return rv;
#else
    nsresult rv;

    mRealListener = listener;

    if (mLoadGroup) {
        nsCOMPtr<nsILoadGroupListenerFactory> factory;
        //
        // Create a load group "proxy" listener...
        //
        rv = mLoadGroup->GetGroupListenerFactory(getter_AddRefs(factory));
        if (factory) {
            nsIStreamListener *newListener;
            rv = factory->CreateLoadGroupListener(mRealListener, &newListener);
            if (NS_SUCCEEDED(rv)) {
                mRealListener = newListener;
                NS_RELEASE(newListener);
            }
        }

        rv = mLoadGroup->AddChannel(this, nsnull);
        if (NS_FAILED(rv)) return rv;
    }

    NS_WITH_SERVICE(nsIFileTransportService, fts, kFileTransportServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = fts->CreateTransportFromStream(mInputStream, "load", nsnull,
                                        getter_AddRefs(mFileTransport));
    if (NS_FAILED(rv)) return rv;

    return mFileTransport->AsyncRead(startPosition, readCount, ctxt, this);
#endif
}

NS_IMETHODIMP
nsInputStreamChannel::AsyncWrite(nsIInputStream *fromStream, PRUint32 startPosition,
                                 PRInt32 writeCount, nsISupports *ctxt,
                                 nsIStreamObserver *observer)
{
    // we don't do output
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsInputStreamChannel::GetLoadAttributes(nsLoadFlags *aLoadAttributes)
{
    *aLoadAttributes = LOAD_NORMAL;
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::SetLoadAttributes(nsLoadFlags aLoadAttributes)
{
    // ignore attempts to set load attributes
    return NS_OK;
}

#define DUMMY_TYPE "text/html"

NS_IMETHODIMP
nsInputStreamChannel::GetContentType(char * *aContentType)
{
    nsresult rv = NS_OK;

    // Parameter validation...
    if (!aContentType) {
        return NS_ERROR_NULL_POINTER;
    }
    *aContentType = nsnull;

    // If we already have a content type, use it.
    if (mContentType) {
        *aContentType = nsCRT::strdup(mContentType);
        if (!*aContentType) {
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
        return rv;
    }

    //
    // No response yet...  Try to determine the content-type based
    // on the file extension of the URI...
    //
    NS_WITH_SERVICE(nsIMIMEService, MIMEService, kMIMEServiceCID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = MIMEService->GetTypeFromURI(mURI, aContentType);
        if (NS_SUCCEEDED(rv)) return rv;
    }

    // if all else fails treat it as text/html?
    if (!*aContentType) 
        *aContentType = nsCRT::strdup(DUMMY_TYPE);
    if (!*aContentType) {
        rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
        rv = NS_OK;
    }

    return rv;
}

NS_IMETHODIMP
nsInputStreamChannel::GetContentLength(PRInt32 *aContentLength)
{
    // The content length is unknown...
    *aContentLength = -1;
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::GetLoadGroup(nsILoadGroup * *aLoadGroup)
{
    *aLoadGroup = mLoadGroup.get();
    NS_IF_ADDREF(*aLoadGroup);
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::GetOwner(nsISupports * *aOwner)
{
    *aOwner = mOwner.get();
    NS_IF_ADDREF(*aOwner);
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::SetOwner(nsISupports * aOwner)
{
    mOwner = aOwner;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsIStreamListener methods:
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsInputStreamChannel::OnStartRequest(nsIChannel* transportChannel, nsISupports* context)
{
    NS_ASSERTION(mRealListener, "No listener...");
    return mRealListener->OnStartRequest(this, context);
}

NS_IMETHODIMP
nsInputStreamChannel::OnStopRequest(nsIChannel* transportChannel, nsISupports* context,
                                    nsresult aStatus, const PRUnichar* aMsg)
{
    nsresult rv;

    rv = mRealListener->OnStopRequest(this, context, aStatus, aMsg);

    if (mLoadGroup) {
        if (NS_SUCCEEDED(rv)) {
            mLoadGroup->RemoveChannel(this, context, aStatus, aMsg);
        }
    }

    // Release the reference to the consumer stream listener...
    mRealListener = null_nsCOMPtr();
    mFileTransport = null_nsCOMPtr();
    return rv;
}

NS_IMETHODIMP
nsInputStreamChannel::OnDataAvailable(nsIChannel* transportChannel, nsISupports* context,
                                      nsIInputStream *aIStream, PRUint32 aSourceOffset,
                                      PRUint32 aLength)
{
    return mRealListener->OnDataAvailable(this, context, aIStream,
                                          aSourceOffset, aLength);
}

////////////////////////////////////////////////////////////////////////////////
