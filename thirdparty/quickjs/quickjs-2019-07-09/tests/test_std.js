import * as std from "std";
import * as os from "os";

function assert(actual, expected, message) {
    if (arguments.length == 1)
        expected = true;

    if (actual === expected)
        return;

    if (actual !== null && expected !== null
    &&  typeof actual == 'object' && typeof expected == 'object'
    &&  actual.toString() === expected.toString())
        return;

    throw Error("assertion failed: got |" + actual + "|" +
                ", expected |" + expected + "|" +
                (message ? " (" + message + ")" : ""));
}

// load more elaborate version of assert if available
try { std.loadScript("test_assert.js"); } catch(e) {}

/*----------------*/

function test_printf()
{
    assert(std.sprintf("a=%d s=%s", 123, "abc"), "a=123 s=abc");
}

function test_file1()
{
    var f, len, str, size, buf, ret, i, str1;

    f = std.tmpfile();
    str = "hello world\n";
    f.puts(str);

    f.seek(0, std.SEEK_SET);
    str1 = f.readAsString();
    assert(str1 === str);
    
    f.seek(0, std.SEEK_END);
    size = f.tell();
    assert(size === str.length);

    f.seek(0, std.SEEK_SET);

    buf = new Uint8Array(size);
    ret = f.read(buf.buffer, 0, size);
    assert(ret === size);
    for(i = 0; i < size; i++)
        assert(buf[i] === str.charCodeAt(i));

    f.close();
}

function test_file2()
{
    var f, str, i, size;
    f = std.tmpfile();
    str = "hello world\n";
    size = str.length;
    for(i = 0; i < size; i++)
        f.putByte(str.charCodeAt(i));
    f.seek(0, std.SEEK_SET);
    for(i = 0; i < size; i++) {
        assert(str.charCodeAt(i) === f.getByte());
    }
    assert(f.getByte() === -1);
    f.close();
}

function test_getline()
{
    var f, line, line_count, lines, i;
    
    lines = ["hello world", "line 1", "line 2" ];
    f = std.tmpfile();
    for(i = 0; i < lines.length; i++) {
        f.puts(lines[i], "\n");
    }

    f.seek(0, std.SEEK_SET);
    assert(!f.eof());
    line_count = 0;
    for(;;) {
        line = f.getline();
        if (line === null)
            break;
        assert(line == lines[line_count]);
        line_count++;
    }
    assert(f.eof());
    assert(line_count === lines.length);

    f.close();
}
 
function test_os()
{
    var fd, fname, buf, buf2, i;

    assert(os.isatty(0));

    fname = "tmp_file.txt";
    fd = os.open(fname, os.O_RDWR | os.O_CREAT | os.O_TRUNC);
    assert(fd >= 0);
    
    buf = new Uint8Array(10);
    for(i = 0; i < buf.length; i++)
        buf[i] = i;
    assert(os.write(fd, buf.buffer, 0, buf.length) === buf.length);

    assert(os.seek(fd, 0, os.SEEK_SET) === 0);
    buf2 = new Uint8Array(buf.length);
    assert(os.read(fd, buf2.buffer, 0, buf2.length) === buf2.length);
    
    for(i = 0; i < buf.length; i++)
        assert(buf[i] == buf2[i]);
    
    assert(os.close(fd) === 0);

    assert(os.remove(fname) === 0);

    fd = os.open(fname, os.O_RDONLY);
    assert(fd < 0);
}

function test_timer()
{
    var th, i;

    /* just test that a timer can be inserted and removed */
    th = [];
    for(i = 0; i < 3; i++)
        th[i] = os.setTimeout(1000, function () { });
    for(i = 0; i < 3; i++)
        os.clearTimeout(th[i]);
}

test_printf();
test_file1();
test_file2();
test_getline();
test_os();
test_timer();
