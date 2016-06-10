webclient = require('../lib/koheron-websocket-client.js')
Command = webclient.Command

class HelloWorld
    constructor : (@kclient) ->
        @device = @kclient.getDevice("HELLO_WORLD")

        @cmds =
            add_42 : @device.getCmdRef( "ADD_42" )

    add42 : (num, cb) ->
        @kclient.readUint32(Command(@device.id, @cmds.add_42, 'I', num), cb)

client = new webclient.KClient('127.0.0.1', 1)

client.init( =>
    hw = new HelloWorld(client)
    hw.add42(58, (res) ->
        console.log res
        process.exit()
    )
)