#include "hypervisor.h"

#include "worker.h"
#include "domain.h"

static Napi::Value dummyCallback(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    return env.Undefined();
}

/******************************************************************************
 * DomainCreateXML                                                            *
 ******************************************************************************/

class DomainCreateXMLWorker : public Worker {
public:

    DomainCreateXMLWorker(
        Napi::Function& callback,
        Napi::Promise::Deferred deferred,
        Hypervisor* hypervisor,
        std::string domainXml
    ) : Worker(callback, deferred, hypervisor),
        domainXml(domainXml) {}

    void Execute(void) override
    {
        domainPtr = virDomainCreateXML(hypervisor->conn, domainXml.c_str(), 0);
        if (!domainPtr) SetVirError();
    }

    void OnOK(void) override
    {
        Napi::HandleScope scope(Env());

        Napi::Object domain = Domain::constructor.New({
            Napi::External<virDomain>::New(Env(), domainPtr) });

        deferred.Resolve(domain);
        Callback().Call({});
    }

private:

    std::string domainXml;
    virDomainPtr domainPtr;

};

Napi::Value Hypervisor::DomainCreateXML(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    Napi::Function callback = Napi::Function::New(env, dummyCallback);

    if (info.Length() <= 0 || !info[0].IsString())
    {
        deferred.Reject(Napi::String::New(env, "Expected a string"));
        return deferred.Promise();
    }

    Napi::String domainXml = info[0].As<Napi::String>();

    DomainCreateXMLWorker* worker =
        new DomainCreateXMLWorker(callback, deferred, this, domainXml);
    worker->Queue();

    return deferred.Promise();
}

/******************************************************************************
 * DomainDefineXML                                                            *
 ******************************************************************************/

class DomainDefineXMLWorker : public Worker {
public:

    DomainDefineXMLWorker(
        Napi::Function& callback,
        Napi::Promise::Deferred deferred,
        Hypervisor* hypervisor,
        std::string domainXml
    ) : Worker(callback, deferred, hypervisor),
        domainXml(domainXml) {}

    void Execute(void) override
    {
        domainPtr = virDomainDefineXML(hypervisor->conn, domainXml.c_str());
        if (!domainPtr) SetVirError();
    }

    void OnOK(void) override
    {
        Napi::HandleScope scope(Env());

        Napi::Object domain = Domain::constructor.New({
            Napi::External<virDomain>::New(Env(), domainPtr) });

        deferred.Resolve(domain);
        Callback().Call({});
    }

private:

    std::string domainXml;
    virDomainPtr domainPtr;

};

Napi::Value Hypervisor::DomainDefineXML(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    Napi::Function callback = Napi::Function::New(env, dummyCallback);

    if (info.Length() <= 0 || !info[0].IsString())
    {
        deferred.Reject(Napi::String::New(env, "Expected a string"));
        return deferred.Promise();
    }

    Napi::String domainXml = info[0].As<Napi::String>();

    DomainDefineXMLWorker* worker =
        new DomainDefineXMLWorker(callback, deferred, this, domainXml);
    worker->Queue();

    return deferred.Promise();
}

/******************************************************************************
 * DomainGetInfo                                                              *
 ******************************************************************************/

class DomainGetInfoWorker : public Worker {
public:

    DomainGetInfoWorker(
        Napi::Function& callback,
        Napi::Promise::Deferred deferred,
        Hypervisor* hypervisor,
        Domain* domain
    ) : Worker(callback, deferred, hypervisor),
        domain(domain) {}

    void Execute(void) override
    {
        int ret = virDomainGetInfo(domain->domainPtr, &domainInfo);
        if (ret < 0) SetVirError();
    }

    void OnOK(void) override
    {
        Napi::HandleScope scope(Env());

        Napi::Object info = Napi::Object::New(Env());
        info.Set("state", Napi::Number::New(Env(), domainInfo.state));
        info.Set("maxMem", Napi::Number::New(Env(), domainInfo.maxMem));
        info.Set("memory", Napi::Number::New(Env(), domainInfo.memory));
        info.Set("nrVirtCpu", Napi::Number::New(Env(), domainInfo.nrVirtCpu));
        info.Set("cpuTime", Napi::Number::New(Env(), domainInfo.cpuTime));

        deferred.Resolve(info);
        Callback().Call({});
    }

private:

    Domain* domain;
    virDomainInfo domainInfo;

};

Napi::Value Hypervisor::DomainGetInfo(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    Napi::Function callback = Napi::Function::New(env, dummyCallback);

    if (info.Length() <= 0 || !info[0].IsObject())
    {
        deferred.Reject(Napi::String::New(env, "Expected an object."));
        return deferred.Promise();
    }

    Domain* domain = Napi::ObjectWrap<Domain>::Unwrap(
        info[0].As<Napi::Object>());

    DomainGetInfoWorker* worker = new DomainGetInfoWorker(
        callback, deferred, this, domain);
    worker->Queue();

    return deferred.Promise();
}

/******************************************************************************
 * DomainGetID                                                                *
 ******************************************************************************/

class DomainGetIDWorker : public Worker {
public:

    DomainGetIDWorker(
        Napi::Function& callback,
        Napi::Promise::Deferred deferred,
        Hypervisor* hypervisor,
        Domain* domain
    ) : Worker(callback, deferred, hypervisor),
        domain(domain) {}

    void Execute(void) override
    {
        id = virDomainGetID(domain->domainPtr);
        if (id == static_cast<unsigned int>(-1)) SetVirError();
    }

    void OnOK(void) override
    {
        Napi::HandleScope scope(Env());
        deferred.Resolve(Napi::Number::New(Env(), this->id));
        Callback().Call({});
    }

private:

    Domain* domain;
    unsigned int id;

};

Napi::Value Hypervisor::DomainGetID(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    Napi::Function callback = Napi::Function::New(env, dummyCallback);

    if (info.Length() <= 0 || !info[0].IsObject())
    {
        deferred.Reject(Napi::String::New(env, "Expected an object."));
        return deferred.Promise();
    }

    Domain* domain = Napi::ObjectWrap<Domain>::Unwrap(
        info[0].As<Napi::Object>());

    DomainGetIDWorker* worker = new DomainGetIDWorker(
        callback, deferred, this, domain);
    worker->Queue();

    return deferred.Promise();
}

/******************************************************************************
 * DomainGetName                                                              *
 ******************************************************************************/

class DomainGetNameWorker : public Worker {
public:

    DomainGetNameWorker(
        Napi::Function& callback,
        Napi::Promise::Deferred deferred,
        Hypervisor* hypervisor,
        Domain* domain
    ) : Worker(callback, deferred, hypervisor),
        domain(domain) {}

    void Execute(void) override
    {
        domainName = virDomainGetName(domain->domainPtr);
        if (domainName == NULL) SetVirError();
    }

    void OnOK(void) override
    {
        Napi::HandleScope scope(Env());
        deferred.Resolve(Napi::String::New(Env(), this->domainName));
        Callback().Call({});
    }

private:

    Domain* domain;
    const char* domainName;

};

Napi::Value Hypervisor::DomainGetName(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    Napi::Function callback = Napi::Function::New(env, dummyCallback);

    if (info.Length() <= 0 || !info[0].IsObject())
    {
        deferred.Reject(Napi::String::New(env, "Expected an object."));
        return deferred.Promise();
    }

    Domain* domain = Napi::ObjectWrap<Domain>::Unwrap(
        info[0].As<Napi::Object>());

    DomainGetNameWorker* worker = new DomainGetNameWorker(
        callback, deferred, this, domain);
    worker->Queue();

    return deferred.Promise();
}

/******************************************************************************
 * DomainGetUUIDString                                                        *
 ******************************************************************************/

class DomainGetUUIDStringWorker : public Worker {
public:

    DomainGetUUIDStringWorker(
        Napi::Function& callback,
        Napi::Promise::Deferred deferred,
        Hypervisor* hypervisor,
        Domain* domain
    ) : Worker(callback, deferred, hypervisor),
        domain(domain) {}

    void Execute(void) override
    {
        int ret = virDomainGetUUIDString(domain->domainPtr, uuid);
        if (ret < 0) SetVirError();
    }

    void OnOK(void) override
    {
        Napi::HandleScope scope(Env());
        deferred.Resolve(Napi::String::New(Env(), this->uuid,
            VIR_UUID_STRING_BUFLEN - 1));
        Callback().Call({});
    }

private:

    Domain* domain;
    char uuid[VIR_UUID_STRING_BUFLEN];

};

Napi::Value Hypervisor::DomainGetUUIDString(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    Napi::Function callback = Napi::Function::New(env, dummyCallback);

    if (info.Length() <= 0 || !info[0].IsObject())
    {
        deferred.Reject(Napi::String::New(env, "Expected an object."));
        return deferred.Promise();
    }

    Domain* domain = Napi::ObjectWrap<Domain>::Unwrap(
        info[0].As<Napi::Object>());

    DomainGetUUIDStringWorker* worker = new DomainGetUUIDStringWorker(
        callback, deferred, this, domain);
    worker->Queue();

    return deferred.Promise();
}

/******************************************************************************
 * DomainLookupByID                                                           *
 ******************************************************************************/

class DomainLookupByIDWorker : public Worker {
public:

    DomainLookupByIDWorker(
        Napi::Function& callback,
        Napi::Promise::Deferred deferred,
        Hypervisor* hypervisor,
        int id
    ) : Worker(callback, deferred, hypervisor),
        id(id) {}

    void Execute(void) override
    {
        domainPtr = virDomainLookupByID(hypervisor->conn, id);
        if (domainPtr == NULL) SetVirError();
    }

    void OnOK(void) override
    {
        Napi::HandleScope scope(Env());

        Napi::Object domain = Domain::constructor.New({
            Napi::External<virDomain>::New(Env(), domainPtr) });

        deferred.Resolve(domain);
        Callback().Call({});
    }

private:

    int id;
    virDomainPtr domainPtr;

};

Napi::Value Hypervisor::DomainLookupByID(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    Napi::Function callback = Napi::Function::New(env, dummyCallback);

    if (info.Length() <= 0 || !info[0].IsNumber())
    {
        deferred.Reject(Napi::String::New(env, "Expected a number."));
        return deferred.Promise();
    }

    int id = info[0].As<Napi::Number>();

    DomainLookupByIDWorker* worker = new DomainLookupByIDWorker(
        callback, deferred, this, id);
    worker->Queue();

    return deferred.Promise();
}

/******************************************************************************
 * DomainLookupByName                                                         *
 ******************************************************************************/

class DomainLookupByNameWorker : public Worker {
public:

    DomainLookupByNameWorker(
        Napi::Function& callback,
        Napi::Promise::Deferred deferred,
        Hypervisor* hypervisor,
        std::string name
    ) : Worker(callback, deferred, hypervisor),
        name(name) {}

    void Execute(void) override
    {
        domainPtr = virDomainLookupByName(hypervisor->conn, name.c_str());
        if (domainPtr == NULL) SetVirError();
    }

    void OnOK(void) override
    {
        Napi::HandleScope scope(Env());

        Napi::Object domain = Domain::constructor.New({
            Napi::External<virDomain>::New(Env(), domainPtr) });

        deferred.Resolve(domain);
        Callback().Call({});
    }

private:

    std::string name;
    virDomainPtr domainPtr;

};

Napi::Value Hypervisor::DomainLookupByName(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    Napi::Function callback = Napi::Function::New(env, dummyCallback);

    if (info.Length() <= 0 || !info[0].IsString())
    {
        deferred.Reject(Napi::String::New(env, "Expected a string."));
        return deferred.Promise();
    }

    std::string name = info[0].As<Napi::String>();

    DomainLookupByNameWorker* worker = new DomainLookupByNameWorker(
        callback, deferred, this, name);
    worker->Queue();

    return deferred.Promise();
}

/******************************************************************************
 * DomainLookupByUUIDString                                                   *
 ******************************************************************************/

class DomainLookupByUUIDStringWorker : public Worker {
public:

    DomainLookupByUUIDStringWorker(
        Napi::Function& callback,
        Napi::Promise::Deferred deferred,
        Hypervisor* hypervisor,
        std::string uuid
    ) : Worker(callback, deferred, hypervisor),
        uuid(uuid) {}

    void Execute(void) override
    {
        domainPtr = virDomainLookupByUUIDString(hypervisor->conn, uuid.c_str());
        if (domainPtr == NULL) SetVirError();
    }

    void OnOK(void) override
    {
        Napi::HandleScope scope(Env());

        Napi::Object domain = Domain::constructor.New({
            Napi::External<virDomain>::New(Env(), domainPtr) });

        deferred.Resolve(domain);
        Callback().Call({});
    }

private:

    std::string uuid;
    virDomainPtr domainPtr;

};

Napi::Value Hypervisor::DomainLookupByUUIDString(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    Napi::Function callback = Napi::Function::New(env, dummyCallback);

    if (info.Length() <= 0 || !info[0].IsString())
    {
        deferred.Reject(Napi::String::New(env, "Expected a string."));
        return deferred.Promise();
    }

    std::string uuid = info[0].As<Napi::String>();

    DomainLookupByUUIDStringWorker* worker = new DomainLookupByUUIDStringWorker(
        callback, deferred, this, uuid);
    worker->Queue();

    return deferred.Promise();
}

/******************************************************************************
 * DomainSave                                                                 *
 ******************************************************************************/

class DomainSaveWorker : public Worker {
public:

    DomainSaveWorker(
        Napi::Function& callback,
        Napi::Promise::Deferred deferred,
        Hypervisor* hypervisor,
        Domain* domain,
        std::string filename
    ) : Worker(callback, deferred, hypervisor),
        domain(domain), filename(filename) {}

    void Execute(void) override
    {
        int ret = virDomainSave(domain->domainPtr, filename.c_str());
        if (ret < 0) SetVirError();
    }

private:

    Domain* domain;
    std::string filename;

};

Napi::Value Hypervisor::DomainSave(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    Napi::Function callback = Napi::Function::New(env, dummyCallback);

    if (info.Length() <= 0 || !info[0].IsObject())
    {
        deferred.Reject(Napi::String::New(env, "Expected an object."));
        return deferred.Promise();
    }
    if (info.Length() <= 1 || !info[1].IsString())
    {
        deferred.Reject(Napi::String::New(env, "Expected a string."));
        return deferred.Promise();
    }

    Domain* domain = Napi::ObjectWrap<Domain>::Unwrap(
        info[0].As<Napi::Object>());
    std::string filename = info[1].As<Napi::String>();

    DomainSaveWorker* worker = new DomainSaveWorker(
        callback, deferred, this, domain, filename);
    worker->Queue();

    return deferred.Promise();
}

/******************************************************************************
 * DomainRestore                                                              *
 ******************************************************************************/

class DomainRestoreWorker : public Worker {
public:

    DomainRestoreWorker(
        Napi::Function& callback,
        Napi::Promise::Deferred deferred,
        Hypervisor* hypervisor,
        std::string filename
    ) : Worker(callback, deferred, hypervisor),
        filename(filename) {}

    void Execute(void) override
    {
        int ret = virDomainRestore(hypervisor->conn, filename.c_str());
        if (ret < 0) SetVirError();
    }

private:

    std::string filename;

};

Napi::Value Hypervisor::DomainRestore(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    Napi::Function callback = Napi::Function::New(env, dummyCallback);

    if (info.Length() <= 0 || !info[0].IsString())
    {
        deferred.Reject(Napi::String::New(env, "Expected a string."));
        return deferred.Promise();
    }

    std::string filename = info[0].As<Napi::String>();

    DomainRestoreWorker* worker = new DomainRestoreWorker(
        callback, deferred, this, filename);
    worker->Queue();

    return deferred.Promise();
}
