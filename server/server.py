import tornado.ioloop
import tornado.web
import sqlite3
import os
import tornado.httpserver
import json
import argparse
import dawnmud_pb2
from google.protobuf.json_format import MessageToJson

verbosity = False
base_dir = "lib/"


class MainHandler(tornado.web.RequestHandler):
    # This is going to show the basic stats page
    def get(self):
        self.render("index.html")

    def post(self):
        self.write("Hello post world")

    def set_default_headers(self):
        self.set_header("Access-Control-Allow-Origin", "*")
        self.set_header("Access-Control-Allow-Headers",
                        "Access-Control-Allow-Headers, Origin, Accept, X-Requested-With, \
                        Content-Type, Access-Control-Request-Method, \
                        Access-Control-Request-Headers")
        self.set_header('Access-Control-Allow-Methods', 'GET, PUT, POST, \
                        DELETE, OPTIONS')
        self.set_header('Cache-Control',
                        'no-store, no-cache, must-revalidate, max-age=0')

    def options(self):
        # no body
        self.set_status(204)
        self.finish()

    # @staticmethod
    # def construct_sql(table_name, json_object):
    #     print("In the construct sql method...")
    #     print("Table name: {}".format(table_name))
    #     columns = ''
    #     values = ''
    #     for key, value in json_object.iteritems():
    #         columns = columns + '\"' + str(key) + '\", '
    #         values = values + '\"' + str(value) + '\", '
    #     c = columns[:-2]
    #     v = values[:-2]
    #     sqlstr = "INSERT INTO {} ({}) VALUES ({})".format(table_name, c, v)
    #     return sqlstr
    #
    # @property
    # def db(self):
    #     return self.application.db
    #
    # @property
    # def cursor(self):
    #     return self.application.cursor
    #
    # @property
    # def table_style(self):
    #     return "<html><head><style>table {font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif;border-collapse: collapse;width: 100%;}table td, #customers th {border: 1px solid #ddd;padding: 8px;}table tr:nth-child(even){background-color: #f2f2f2;}table tr:hover {background-color: #ddd;}table th {padding-top: 12px;padding-bottom: 12px;text-align: left;background-color: #4CAF50;color: white;}</style></head>"


class FetchZonesHandler(MainHandler):
    def initialize(self):
        pass

    def get(self):
        self.set_header("Content-Type", "text/json")

        # Fetch zones here
        zone_list = dawnmud_pb2.ZoneList()

        f = open(base_dir + "/zones", "rb")
        zone_list.ParseFromString(f.read())
        f.close()

        jsonZone = []
        for zone in zone_list.zone:
            jsonZone.append(MessageToJson(zone))
            print jsonZone

        # for zone in zone_list.zone:
        #     print "Name: ", zone.name
        #     print "Builders: ", zone.builders
        #     print "Lifespan: ", zone.lifespan
        #     print "Age: ", zone.age
        #     print "Bottom: ", zone.bot
        #     print "Top: ", zone.top
        #     print "Reset mode: ", zone.reset_mode
        #     print "Number: ", zone.number
        #     print ""
        #     print ""
        results = {"results": jsonZone}
        # self.write(json.dumps(jsonZone))
        self.write(results)

    def post(self):
        self.set_header("Content-Type", "text/plain")

        # Fetch zones here
        zone_list = dawnmud_pb2.ZoneList()

        f = open(base_dir + "/zones", "rb")
        zone_list.ParseFromString(f.read())
        f.close()

        for zone in zone_list.zone:
            print "Name: ", zone.name
            print "Builders: ", zone.builders
            print "Lifespan: ", zone.lifespan
            print "Age: ", zone.age
            print "Bottom: ", zone.bot
            print "Top: ", zone.top
            print "Reset mode: ", zone.reset_mode
            print "Number: ", zone.number
            print ""
            print ""
        results = {"results": True}
        self.write(results)


class Application(tornado.web.Application):
    def __init__(self):
        tornado_settings = {
            "static_path": os.path.join(os.path.dirname(__file__), "static"),
            'debug': True
        }

        handlers = [
            (r"/", MainHandler),
            (r"/get_zone_information/", FetchZonesHandler),
            (r"/js/(.*)", tornado.web.StaticFileHandler,
             dict(path=tornado_settings['static_path'])),
            (r"/(styles\.css)", tornado.web.StaticFileHandler,
             dict(path=tornado_settings['static_path'])),
            (r"/(favicon\.ico)", tornado.web.StaticFileHandler,
             dict(path=tornado_settings['static_path'])),
            (r"/img/(.*)", tornado.web.StaticFileHandler,
             dict(path=tornado_settings['static_path']))
        ]

        tornado.web.Application.__init__(self, handlers, **tornado_settings)

        # self.db = database.Connection()
        # self.db = sqlite3.connect('time_keep_database.db')
        # self.cursor = self.db.cursor()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("world_files", help="The parent directory of the world files.")
    parser.add_argument("-v", "--verbose", help="increase output verbosity", action="store_true")
    args = parser.parse_args()

    # Optionally turn on verbosity
    if args.verbose:
        verbosity = True
    # Set the base directory for the serialized files
    # Need a better way to do this
    base_dir = args.world_files

    settings = {}
    http_server = tornado.httpserver.HTTPServer(Application())
    http_server.listen(8090)
    tornado.ioloop.IOLoop.instance().start()
