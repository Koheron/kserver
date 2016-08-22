# Websocket client tests
# (c) Koheron

# http://stackoverflow.com/questions/19971713/nodeunit-runtime-thrown-errors-in-test-function-are-silent/20038890#20038890
process.on('uncaughtException', (err) ->
  console.error err.stack
)

websock_client = require('../lib/koheron-websocket-client.js')
Command = websock_client.Command

class Tests
    "use strict"

    constructor : (@kclient) ->
        @device = @kclient.getDevice("Tests")
        @id = @device.id
        @cmds = @device.getCmds()

    sendManyParams : (u1, u2, f, b, cb) ->
        @kclient.readBool(Command(@id, @cmds.rcv_many_params, 'IIf?', u1, u2, f, b), cb)

    setFloat : (f, cb) ->
        @kclient.readBool(Command(@id, @cmds.set_float, 'f', f), cb)

    setDouble : (d, cb) ->
        @kclient.readBool(Command(@id, @cmds.set_double, 'd', d), cb)

    setUnsigned : (u8, u16, u32, cb) ->
        @kclient.readBool(Command(@id, @cmds.set_unsigned, 'BHI', u8, u16, u32), cb)

    setSigned : (i8, i16, i32, cb) ->
        @kclient.readBool(Command(@id, @cmds.set_signed, 'bhi', i8, i16, i32), cb)

    rcvStdVector : (cb) ->
        @kclient.readFloat32Array(Command(@id, @cmds.send_std_vector), cb)

    rcvStdArray : (cb) ->
        @kclient.readFloat32Array(Command(@id, @cmds.send_std_array), cb)

    rcvCArray1 : (len, cb) ->
        @kclient.readFloat32Array(Command(@id, @cmds.send_c_array1, 'I', len), cb)

    rcvCArray2 : (cb) ->
        @kclient.readFloat32Array(Command(@id, @cmds.send_c_array2), cb)

    sendBuffer : (len, cb) ->
        buffer = new Uint32Array(len)
        buffer[i] = i*i for i in [0..len]
        @kclient.sendArray(Command(@id, @cmds.set_buffer, 'I', buffer.length), buffer, (ok) -> cb(ok==1))

    sendStdArray : (cb) ->
        u = 4223453
        f = 3.141592
        d = 2.654798454646
        i = -56789
        array = new Uint32Array(8192)
        array[_i] = _i for _i in [0..array.length]
        @kclient.readBool(Command(@id, @cmds.rcv_std_array, 'IfAdi', u, f, array, d, i), cb)

    sendStdArray2 : (cb) ->
        array = new Float32Array(8192)
        array[i] = Math.log(i + 1) for i in [0..array.length]
        @kclient.readBool(Command(@id, @cmds.rcv_std_array2, 'A', array), cb)

    sendStdArray3 : (cb) ->
        array = new Float64Array(8192)
        array[i] = Math.sin(i) for i in [0..array.length]
        @kclient.readBool(Command(@id, @cmds.rcv_std_array3, 'A', array), cb)

    readUint : (cb) ->
        @kclient.readUint32(Command(@id, @cmds.read_uint), cb)

    readInt : (cb) ->
        @kclient.readInt32(Command(@id, @cmds.read_int), cb)

    readFloat : (cb) ->
        @kclient.readFloat32(Command(@id, @cmds.read_float), cb)

    readDouble : (cb) ->
        @kclient.readFloat64(Command(@id, @cmds.read_double), cb)

    readString : (cb) ->
        @kclient.readString(Command(@id, @cmds.get_cstr), cb)

    readStdString : (cb) ->
        @kclient.readString(Command(@id, @cmds.get_std_string), cb)

    readJSON : (cb) ->
        @kclient.readJSON(Command(@id, @cmds.get_json), cb)

    readJSON2 : (cb) ->
        @kclient.readJSON(Command(@id, @cmds.get_json2), cb)

    readTuple : (cb) ->
        @kclient.readTuple(Command(@id, @cmds.get_tuple), 'Idd?', cb)

    readTuple3 : (cb) ->
        @kclient.readTuple(Command(@id, @cmds.get_tuple3), '?ffBH', cb)

    readTuple4 : (cb) ->
        @kclient.readTuple(Command(@id, @cmds.get_tuple4), 'bbhhii', cb)

# Unit tests

exports.triggerServerBroadcast = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 2)
    assert.expect(3)

    assert.doesNotThrow( =>
        client.init( =>
            client.subscribeServerBroadcast( (channel, event_id) =>
                assert.equals(channel, 0)
                assert.equals(event_id, 0)
                client.exit()
                assert.done()
            )
            client.broadcastPing()
        )
    )

exports.sendManyParams = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.sendManyParams(429496729, 2048, 3.14, true, (is_ok) =>
                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.readUint = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readUint( (num) =>
                assert.equals(num, 301062138)
                client.exit()
                assert.done()
            )
        )
    )

exports.readInt = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readInt( (num) =>
                assert.equals(num, -214748364)
                client.exit()
                assert.done()
            )
        )
    )

exports.readFloat = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readFloat( (num) =>
                assert.ok(Math.abs(num - 3.141592) < 1e-7)
                client.exit()
                assert.done()
            )
        )
    )

exports.readDouble = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readDouble( (num) =>
                assert.ok(Math.abs(num - 2.2250738585072009) < 1e-14)
                client.exit()
                assert.done()
            )
        )
    )

exports.setFloat = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.setFloat( 12.5, (is_ok) =>
                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.setDouble = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.setDouble( 1.428571428571428492127, (is_ok) =>
                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.setUnsigned = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.setUnsigned(255, 65535, 4294967295, (is_ok) =>
                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.setSigned = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.setSigned(-125, -32764, -2147483645, (is_ok) =>
                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.rcvStdVector = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.rcvStdVector( (array) =>
                is_ok = true
                for i in [0..array.length-1]
                    if array[i] != i*i*i
                        is_ok = false
                        break

                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.rcvStdArray = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.rcvStdArray( (array) =>
                is_ok = true
                for i in [0..array.length-1]
                    if array[i] != i*i
                        is_ok = false
                        break

                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.rcvCArray1 = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(3)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            len = 10
            tests.rcvCArray1(len, (array) =>
                assert.equal(array.length, 2*len)
                is_ok = true
                for i in [0..2*len-1]
                    if array[i] != i/2
                        is_ok = false
                        break

                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.rcvCArray2 = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(3)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.rcvCArray2((array) =>
                assert.equal(array.length, 10)
                is_ok = true
                for i in [0..10-1]
                    if array[i] != i/4
                        is_ok = false
                        break

                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.sendBuffer = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            len = 10
            tests.sendBuffer(len, (is_ok) =>
                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.sendStdArray = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.sendStdArray( (is_ok) =>
                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.sendStdArray2 = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.sendStdArray2( (is_ok) =>
                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.sendStdArray3 = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.sendStdArray3( (is_ok) =>
                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.readString = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readString( (str) =>
                assert.equals(str, 'Hello !')
                client.exit()
                assert.done()
            )
        )
    )

exports.readStdString = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readStdString( (str) =>
                assert.equals(str, 'Hello World !')
                client.exit()
                assert.done()
            )
        )
    )

exports.readJSON = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(11)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readJSON( (data) =>
                assert.ok(data.date?)
                assert.ok(data.machine?)
                assert.ok(data.time?)
                assert.ok(data.user?)
                assert.ok(data.version?)
                assert.equal(data.date, '20/07/2016')
                assert.equal(data.machine, 'PC-3')
                assert.equal(data.time, '18:16:13')
                assert.equal(data.user, 'thomas')
                assert.equal(data.version, '0691eed')
                client.exit()
                assert.done()
            )
        )
    )

exports.readJSON2 = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(17)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readJSON2( (data) =>
                assert.ok(data.firstName?)
                assert.ok(data.lastName?)
                assert.ok(data.age?)
                assert.ok(data.phoneNumber?)
                assert.equal(data.firstName, 'John')
                assert.equal(data.lastName, 'Smith')
                assert.equal(data.age, 25)
                assert.equal(data.phoneNumber.length, 2)
                assert.ok(data.phoneNumber[0].type?)
                assert.ok(data.phoneNumber[0].number?)
                assert.ok(data.phoneNumber[1].type?)
                assert.ok(data.phoneNumber[1].number?)
                assert.equal(data.phoneNumber[0].type, 'home')
                assert.equal(data.phoneNumber[0].number, '212 555-1234')
                assert.equal(data.phoneNumber[1].type, 'fax')
                assert.equal(data.phoneNumber[1].number, '646 555-4567')
                client.exit()
                assert.done()
            )
        )
    )

exports.readTuple = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(5)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readTuple( (tuple) =>
                assert.equals(tuple[0], 501762438)
                assert.ok(Math.abs(tuple[1] - 507.3858) < 5e-6)
                assert.ok(Math.abs(tuple[2] - 926547.6468507200) < 1e-14)
                assert.ok(tuple[3])
                client.exit()
                assert.done()
            )
        )
    )

exports.readTuple3 = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(6)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readTuple3( (tuple) =>
                assert.ok(not tuple[0])
                assert.ok(Math.abs(tuple[1] - 3.14159) < 1e-6)
                assert.ok(Math.abs(tuple[2] - 507.3858) < 5e-6)
                assert.equals(tuple[3], 42)
                assert.equals(tuple[4], 6553)
                client.exit()
                assert.done()
            )
        )
    )

exports.readTuple4 = (assert) ->
    client = new websock_client.KClient('127.0.0.1', 1)
    assert.expect(7)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readTuple4( (tuple) =>
                assert.equals(tuple[0], -127)
                assert.equals(tuple[1], 127)
                assert.equals(tuple[2], -32767)
                assert.equals(tuple[3], 32767)
                assert.equals(tuple[4], -2147483647)
                assert.equals(tuple[5], 2147483647)
                client.exit()
                assert.done()
            )
        )
    )
