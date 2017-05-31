#include <mbgl/storage/default_file_source.hpp>
#include <mbgl/storage/asset_file_source.hpp>
#include <mbgl/storage/local_file_source.hpp>
#include <mbgl/storage/online_file_source.hpp>
#include <mbgl/storage/offline_database.hpp>
#include <mbgl/storage/offline_download.hpp>

#include <mbgl/actor/actor.hpp>
#include <mbgl/actor/actor_ref.hpp>
#include <mbgl/actor/mailbox.hpp>
#include <mbgl/util/platform.hpp>
#include <mbgl/util/url.hpp>
#include <mbgl/util/work_request.hpp>
#include <mbgl/util/threaded_run_loop.hpp>
#include <mbgl/util/logging.hpp>

#include <cassert>

namespace {

const std::string assetProtocol = "asset://";

bool isAssetURL(const std::string& url) {
    return std::equal(assetProtocol.begin(), assetProtocol.end(), url.begin());
}

} // namespace

namespace mbgl {

class DefaultFileSource::Impl {
public:
    Impl(ActorRef<Impl>) {}

    void initialize(std::shared_ptr<FileSource> assetFileSource_, const std::string& cachePath, uint64_t maximumCacheSize) {
        d = std::make_unique<Data>(assetFileSource_, cachePath, maximumCacheSize);
    }

    void destroy(std::promise<void> promise) {
        d.reset();
        promise.set_value();
    }

    void setAPIBaseURL(const std::string& url) {
        d->onlineFileSource.setAPIBaseURL(url);
    }

    void setAccessToken(const std::string& accessToken) {
        d->onlineFileSource.setAccessToken(accessToken);
    }

    void setResourceTransform(OnlineFileSource::ResourceTransform&& transform) {
        d->onlineFileSource.setResourceTransform(std::move(transform));
    }

    void listRegions(std::function<void (std::exception_ptr, optional<std::vector<OfflineRegion>>)> callback) {
        try {
            callback({}, d->offlineDatabase.listRegions());
        } catch (...) {
            callback(std::current_exception(), {});
        }
    }

    void createRegion(const OfflineRegionDefinition& definition,
                      const OfflineRegionMetadata& metadata,
                      std::function<void (std::exception_ptr, optional<OfflineRegion>)> callback) {
        try {
            callback({}, d->offlineDatabase.createRegion(definition, metadata));
        } catch (...) {
            callback(std::current_exception(), {});
        }
    }

    void updateMetadata(const int64_t regionID,
                      const OfflineRegionMetadata& metadata,
                      std::function<void (std::exception_ptr, optional<OfflineRegionMetadata>)> callback) {
        try {
            callback({}, d->offlineDatabase.updateMetadata(regionID, metadata));
        } catch (...) {
            callback(std::current_exception(), {});
        }
    }

    void getRegionStatus(int64_t regionID, std::function<void (std::exception_ptr, optional<OfflineRegionStatus>)> callback) {
        try {
            callback({}, getDownload(regionID).getStatus());
        } catch (...) {
            callback(std::current_exception(), {});
        }
    }

    void deleteRegion(OfflineRegion&& region, std::function<void (std::exception_ptr)> callback) {
        try {
            d->downloads.erase(region.getID());
            d->offlineDatabase.deleteRegion(std::move(region));
            callback({});
        } catch (...) {
            callback(std::current_exception());
        }
    }

    void setRegionObserver(int64_t regionID, std::unique_ptr<OfflineRegionObserver> observer) {
        getDownload(regionID).setObserver(std::move(observer));
    }

    void setRegionDownloadState(int64_t regionID, OfflineRegionDownloadState state) {
        getDownload(regionID).setState(state);
    }

    void request(AsyncRequest* req, Resource resource, Callback callback) {
        if (isAssetURL(resource.url)) {
            //Asset request
            d->tasks[req] = d->assetFileSource->request(resource, callback);
        } else if (LocalFileSource::acceptsURL(resource.url)) {
            //Local file request
            d->tasks[req] = d->localFileSource->request(resource, callback);
        } else {
            // Try the offline database
            Resource revalidation = resource;

            const bool hasPrior = resource.priorEtag || resource.priorModified || resource.priorExpires;
            if (!hasPrior || resource.necessity == Resource::Optional) {
                auto offlineResponse = d->offlineDatabase.get(resource);

                if (resource.necessity == Resource::Optional && !offlineResponse) {
                    // Ensure there's always a response that we can send, so the caller knows that
                    // there's no optional data available in the cache.
                    offlineResponse.emplace();
                    offlineResponse->noContent = true;
                    offlineResponse->error = std::make_unique<Response::Error>(
                            Response::Error::Reason::NotFound, "Not found in offline database");
                }

                if (offlineResponse) {
                    revalidation.priorModified = offlineResponse->modified;
                    revalidation.priorExpires = offlineResponse->expires;
                    revalidation.priorEtag = offlineResponse->etag;
                    callback(*offlineResponse);
                }
            }

            // Get from the online file source
            if (resource.necessity == Resource::Required) {
                d->tasks[req] = d->onlineFileSource.request(revalidation, [=] (Response onlineResponse) {
                    this->d->offlineDatabase.put(revalidation, onlineResponse);
                    callback(onlineResponse);
                });
            }
        }
    }

    void cancel(AsyncRequest* req) {
        d->tasks.erase(req);
    }

    void setOfflineMapboxTileCountLimit(uint64_t limit) {
        d->offlineDatabase.setOfflineMapboxTileCountLimit(limit);
    }

    void put(const Resource& resource, const Response& response) {
        d->offlineDatabase.put(resource, response);
    }

private:
    OfflineDownload& getDownload(int64_t regionID) {
        auto it = d->downloads.find(regionID);
        if (it != d->downloads.end()) {
            return *it->second;
        }
        return *d->downloads.emplace(regionID,
            std::make_unique<OfflineDownload>(regionID, d->offlineDatabase.getRegionDefinition(regionID), d->offlineDatabase, d->onlineFileSource)).first->second;
    }

    // Lazy initialized, so it gets constructed on the scheduler thread.
    struct Data {
        Data(std::shared_ptr<FileSource> assetFileSource_, const std::string& cachePath, uint64_t maximumCacheSize)
                : assetFileSource(assetFileSource_)
                , localFileSource(std::make_unique<LocalFileSource>())
                , offlineDatabase(cachePath, maximumCacheSize) {}

        const std::shared_ptr<FileSource> assetFileSource;
        const std::unique_ptr<FileSource> localFileSource;
        OfflineDatabase offlineDatabase;
        OnlineFileSource onlineFileSource;
        std::unordered_map<AsyncRequest*, std::unique_ptr<AsyncRequest>> tasks;
        std::unordered_map<int64_t, std::unique_ptr<OfflineDownload>> downloads;
    };

    std::unique_ptr<Data> d;
};

DefaultFileSource::DefaultFileSource(const std::string& cachePath,
                                     const std::string& assetRoot,
                                     uint64_t maximumCacheSize)
    : DefaultFileSource(cachePath, std::make_unique<AssetFileSource>(assetRoot), maximumCacheSize) {
}

DefaultFileSource::DefaultFileSource(const std::string& cachePath,
                                     std::unique_ptr<FileSource>&& assetFileSource_,
                                     uint64_t maximumCacheSize)
        : threadedRunLoop(std::make_unique<ThreadedRunLoop>("DefaultFileSource"))
        , assetFileSource(std::move(assetFileSource_))
        , thread(std::make_unique<Actor<Impl>>(*threadedRunLoop)) {
    thread->invoke(&Impl::initialize, assetFileSource, cachePath, maximumCacheSize);
}

DefaultFileSource::~DefaultFileSource() {
    std::promise<void> sync;
    auto future = sync.get_future();

    thread->invoke(&Impl::destroy, std::move(sync));
    future.get();
};

void DefaultFileSource::setAPIBaseURL(const std::string& baseURL) {
    thread->invoke(&Impl::setAPIBaseURL, baseURL);

    {
        std::lock_guard<std::mutex> lock(cachedBaseURLMutex);
        cachedBaseURL = baseURL;
    }
}

std::string DefaultFileSource::getAPIBaseURL() {
    std::lock_guard<std::mutex> lock(cachedBaseURLMutex);
    return cachedBaseURL;
}

void DefaultFileSource::setAccessToken(const std::string& accessToken) {
    thread->invoke(&Impl::setAccessToken, accessToken);

    {
        std::lock_guard<std::mutex> lock(cachedAccessTokenMutex);
        cachedAccessToken = accessToken;
    }
}

std::string DefaultFileSource::getAccessToken() {
    std::lock_guard<std::mutex> lock(cachedAccessTokenMutex);
    return cachedAccessToken;
}

void DefaultFileSource::setResourceTransform(std::function<std::string(Resource::Kind, std::string&&)>) {
    //if (transform) {
    //    auto loop = util::RunLoop::Get();
    //    thread->invoke(&Impl::setResourceTransform, [loop, transform](Resource::Kind kind_, std::string&& url_, auto callback_) {
    //        return loop->invokeWithCallback([transform](Resource::Kind kind, std::string&& url, auto callback) {
    //            callback(transform(kind, std::move(url)));
    //        }, kind_, std::move(url_), callback_);
    //    });
    //} else {
    //    thread->invoke(&Impl::setResourceTransform, nullptr);
    //}
}

std::unique_ptr<AsyncRequest> DefaultFileSource::request(const Resource& resource, Callback callback) {
    class DefaultFileRequest : public AsyncRequest {
    public:
        DefaultFileRequest(Resource resource_, FileSource::Callback callback, ActorRef<DefaultFileSource::Impl> fs_)
            : mailbox(std::make_shared<Mailbox>(*util::RunLoop::Get()))
            , fs(fs_) {
            fs.invoke(&DefaultFileSource::Impl::request, this, resource_, [callback, ref = ActorRef<DefaultFileRequest>(*this, mailbox)](Response res) mutable {
                ref.invoke(&DefaultFileRequest::runCallback, callback, res);
            });
        }

        ~DefaultFileRequest() override {
            fs.invoke(&DefaultFileSource::Impl::cancel, this);
        }

        void runCallback(FileSource::Callback callback, Response res) {
            callback(res);
        }

    private:
        std::shared_ptr<Mailbox> mailbox;
        ActorRef<DefaultFileSource::Impl> fs;
    };

    return std::make_unique<DefaultFileRequest>(resource, callback, *thread);
}

void DefaultFileSource::listOfflineRegions(std::function<void (std::exception_ptr, optional<std::vector<OfflineRegion>>)> callback) {
    thread->invoke(&Impl::listRegions, callback);
}

void DefaultFileSource::createOfflineRegion(const OfflineRegionDefinition& definition,
                                            const OfflineRegionMetadata& metadata,
                                            std::function<void (std::exception_ptr, optional<OfflineRegion>)> callback) {
    thread->invoke(&Impl::createRegion, definition, metadata, callback);
}

void DefaultFileSource::updateOfflineMetadata(const int64_t regionID,
                                            const OfflineRegionMetadata& metadata,
                                            std::function<void (std::exception_ptr, optional<OfflineRegionMetadata>)> callback) {
    thread->invoke(&Impl::updateMetadata, regionID, metadata, callback);
}

void DefaultFileSource::deleteOfflineRegion(OfflineRegion&& region, std::function<void (std::exception_ptr)> callback) {
    thread->invoke(&Impl::deleteRegion, std::move(region), callback);
}

void DefaultFileSource::setOfflineRegionObserver(OfflineRegion& region, std::unique_ptr<OfflineRegionObserver> observer) {
    thread->invoke(&Impl::setRegionObserver, region.getID(), std::move(observer));
}

void DefaultFileSource::setOfflineRegionDownloadState(OfflineRegion& region, OfflineRegionDownloadState state) {
    thread->invoke(&Impl::setRegionDownloadState, region.getID(), state);
}

void DefaultFileSource::getOfflineRegionStatus(OfflineRegion& region, std::function<void (std::exception_ptr, optional<OfflineRegionStatus>)> callback) const {
    thread->invoke(&Impl::getRegionStatus, region.getID(), callback);
}

void DefaultFileSource::setOfflineMapboxTileCountLimit(uint64_t) const {
    //thread->invokeSync(&Impl::setOfflineMapboxTileCountLimit, limit);
}

void DefaultFileSource::pause() {
    //thread->pause();
}

void DefaultFileSource::resume() {
    //thread->resume();
}

// For testing only:

void DefaultFileSource::put(const Resource&, const Response&) {
    //thread->invokeSync(&Impl::put, resource, response);
}

} // namespace mbgl
