#include <rocketjoe/services/lua_engine/lua_sandbox.hpp>
#include <map>
#include <api/http.hpp>
#include <api/websocket.hpp>

namespace rocketjoe { namespace services { namespace lua_engine {

                constexpr const char * write = "write";

                auto load_libraries( sol::state&lua,const std::map<std::string,std::string>&env) -> void {

                    lua.open_libraries(
                            sol::lib::base,
                            sol::lib::table,
                            sol::lib::package,
                            sol::lib::coroutine,
                            sol::lib::math,
                            sol::lib::utf8,
                            sol::lib::string
                    );

                    for(const auto&i:env){
                        lua.require_file(i.first,i.second);
                    }

                }


                auto object_to_lua_table (api::transport& object, sol::table& table) ->void {
                    switch (object->type()) {
                        case api::transport_type::http: {
                            auto *http_ = static_cast<api::http *>(object.get());
                            table["type"] = "http";
                            table["uri"] = http_->uri();
                            ///headers
                            for(auto&i :*http_){
                                ///std::cerr << "heder_key = "<< i.first <<" header_value "<< i.second <<std::endl;
                                table[i.first]=i.second;
                            }
                            ///TODO Header / body not copy add method json_body_get
                            table["body"] = http_->body();

                            break;
                        }
                        case api::transport_type::ws: {
                            table["type"] = "ws";
                            break;
                        }
                    }

                }


                auto lua_table_to_object (sol::table& table,api::transport& object) ->void {
                    switch (object->type()) {
                        case api::transport_type::http: {
                            auto *http_ = static_cast<api::http *>(object.get());
                            auto header =  table["header"].get<sol::table>();
                            for(auto&i:header){
                                std::cerr<< "header_key" <<i.first.as<std::string>() << "header_value"<<i.second.as<std::string>()<<std::endl;
                                http_->header(i.first.as<std::string>(),i.second.as<std::string>());

                            }
                            http_->body(table["body"].get<std::string>());

                            break;
                        }
                        case api::transport_type::ws: {
                            table["type"] = "ws";
                            break;
                        }
                    }

                }

                lua_context::lua_context(actor_zeta::behavior::context_t& ptr):context_(ptr) {

                }

                auto lua_context::run() -> void {
                    exuctor = std::make_unique<std::thread>(
                            [this]() {
                                r();
                            }
                    );
                }

                auto lua_context::push_job(api::transport &&job) -> void {
                    device_.push(std::move(job));
                }

            auto lua_context::environment_configuration(const std::string &name,const std::map<std::string,std::string> & env) -> void {

                load_libraries(lua,env);

                lua.set_function(
                        "jobs_wait",
                        [this](sol::table jobs) {
                            ///std::cerr<<device_.pop()<<std::endl;
                            auto job_id = device_.pop();
                            if(job_id != 0){
                                std::cerr<<job_id<<std::endl;
                                jobs[1] = job_id;
                            }

                        }
                );
                /// C++ -> lua owener
                lua.set_function(
                        "job_read",
                        [this](std::size_t id,sol::table response) -> void {
                            if (device_.in(id)) {
                                auto &transport = device_.get(id);
                                object_to_lua_table(transport,response);
                            }
                        }
                );

                ////lua -> C++ owener
                lua.set_function(
                        "job_write_and_close",
                        [this](sol::table response){
                            auto id =  response["id"].get<size_t >();
                            auto type =  response["type"].get<std::string>();

                            auto in_put = device_.get(id)->id();
                            std::cerr<<"id = " <<id << "type = "<<type<<std::endl;

                            if("http" == type){
                                auto http = api::make_transport<api::http>(in_put);
                                lua_table_to_object(response,http);

                                context_.addresses("http")->send(
                                        actor_zeta::messaging::make_message(
                                                context_.self(),
                                                write,
                                                std::move(http)
                                        )
                                );
                            } else if ("ws" == type){
                                auto ws = api::make_transport<api::web_socket>(id);
                                lua_table_to_object(response,ws);
                                context_.addresses("ws")->send(
                                        actor_zeta::messaging::make_message(
                                                context_.self(),
                                                write,
                                                std::move(ws)
                                        )
                                );
                            }



                        }
                );

                r = lua.load_file(name);
            }

}}}