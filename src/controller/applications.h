//
// Created by thuanqin on 16/6/17.
//

#ifndef OGP_CONTROLLER_APPLICATIONS_H
#define OGP_CONTROLLER_APPLICATIONS_H

#include <ctime>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "common/log.h"

class AppVersion {
public:
    AppVersion(int id, int app_id, const std::string &version, const std::string &registe_time,
               const std::string &description) : id(id), app_id(app_id), version(version), registe_time(registe_time),
                                                 description(description) { }

    int get_id() { return id;}
    int get_app_id() {return app_id;}
    std::string get_version() {return version;}
    std::string get_registe_time() {return registe_time;}
    std::string get_description() {return description;}

private:
    int id;
    int app_id;
    std::string version;
    std::string registe_time;
    std::string description;
};
typedef std::shared_ptr<AppVersion> app_version_ptr;

class Application {
public:

    Application(int id, const std::string &source,
                const std::string &name, const std::string &description) : id(id),
                                                                           source(source),
                                                                           name(name),
                                                                           description(description) { }
    int get_id() {return id;}
    std::string get_source() {return source;}
    std::string get_name() {return name;}
    std::string get_description() {return description;}
    void add_version(app_version_ptr v) {
        versions_lock.lock();
        versions.push_back(v);
        versions_lock.unlock();
    }
    std::vector<app_version_ptr> get_versions() {return versions;}
    app_version_ptr get_version(std::string version) {
        app_version_ptr ver = nullptr;
        versions_lock.lock();
        for (auto v: versions) {
            if (v->get_version() == version) {
                ver = v;
                break;
            }
        }
        versions_lock.unlock();
        return ver;
    }

private:
    int id;
    std::string source;
    std::string name;
    std::string description;
    std::vector<app_version_ptr> versions;
    std::mutex versions_lock;
};
typedef std::shared_ptr<Application> application_ptr;

class ApplicationsBase {
public:
    virtual ~ApplicationsBase() {}
    virtual void add_application(application_ptr a) = 0;
    virtual application_ptr get_application(int app_id) = 0;
    virtual application_ptr get_application(std::string app_name) = 0;
    virtual std::vector<application_ptr> get_applications() = 0;
};

class Applications: public ApplicationsBase {
public:
    ~Applications() {}

    void add_application(application_ptr a) {
        applications_lock.lock();
        applications.push_back(a);
        applications_lock.unlock();
    }

    application_ptr get_application(int app_id) {
        application_ptr a = nullptr;
        applications_lock.lock();
        for (auto app: applications) {
            if (app->get_id() == app_id) {
                a = app;
                break;
            }
        }
        applications_lock.unlock();
        return a;
    }

    application_ptr get_application(std::string app_name) {
        application_ptr a = nullptr;
        applications_lock.lock();
        for (auto app: applications) {
            if (app->get_name() == app_name) {
                a = app;
                break;
            }
        }
        applications_lock.unlock();
        return a;
    }
    std::vector<application_ptr> get_applications() {return applications;}
private:
    std::vector<application_ptr> applications;
    std::mutex applications_lock;
};

#endif //OGP_CONTROLLER_APPLICATIONS_H
