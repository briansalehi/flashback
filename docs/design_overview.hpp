/**
 * \page design_overview Design Overview
 *
 * @startuml
 *
 * interface user_interface {
 * }
 *
 * class gui {
 * }
 *
 * class cli {
 * }
 *
 * user_interface <|-- cli
 * user_interface <|-- gui
 *
 *
 * interface connection {
 * }
 *
 * class network {
 * }
 *
 * connection <|-- network
 *
 * interface storage {
 * }
 *
 * class cache {
 * }
 *
 * storage <|-- cache
 *
 * class client {
 *     - user_interface
 *     - connection
 *     - storage
 * }
 *
 * client *-- user_interface
 * client *-- connection
 * client *-- storage
 *
 * cloud Internet {
 *     class server {
 *     }
 *
 *     interface database {
 *     }
 *
 *     class postgres {
 *     }
 *
 *     database <|- postgres
 * }
 *
 * server <--> client: grpc
 * server <-> database : grpc
 *
 * @enduml
 */
