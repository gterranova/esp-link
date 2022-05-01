//var Vulcanize = require('vulcanize');
var minify = require('./node_modules/minify/lib/minify.js');
//const { snakeCase } = require('snake-case');
var zlib = require('zlib');
var fs = require('fs');
var path = require('path');
const mkdirp = require('mkdirp');
//const del = require('del');
const stream = require("stream");
//var streamPass = new stream.PassThrough();
//const outFolder = path.resolve('./include').replace(/\\/g, '/');

const cmdArgs = process.argv.slice(2);
const inputFolder = path.resolve(cmdArgs[0]);
const generatedFolder = path.resolve(cmdArgs[1]);
mkdirp.sync(generatedFolder);

const filelist = [];

const minifyOptions = {
    html: {
        continueOnParseError: false,
        collapseWhitespace: true,
        removeAttributeQuotes: false,
    },
    css: {
        continueOnParseError: false,
        compatibility: '*',
    },
    js: {
        ecma: 5,
    },
    img: {
        maxSize: 4096,
    },
};

//var vulcan = new Vulcanize({inlineScripts: true, inlineCss: true, stripComments: true, abspath: inputFolder });

//var destination = outFolder + '/webpages.gz.h';

/*
var wstream = fs.createWriteStream(destination);
wstream.on('error', function (err) {
    throw new Error(err);
});

wstream.write(`#include "c_types.h"

typedef struct filelist_t {
    const char *name;
    uint8 *content;
    unsigned int size;
    bool compressed;
} filelist_t;

`);

function convert(source, data) {

    //console.log(`Converting ${source}`);
    var data = data !== undefined? data: fs.readFileSync(source.replace(/\\/g, '/'));

    wstream.write(`#define ${snakeCase(path.basename(source))}_len ` + data.length + '\n');
    wstream.write(`const uint8_t ${snakeCase(path.basename(source))}[] = {`)

    for (i=0; i<data.length; i++) {
        if (i % 32 == 0) wstream.write("\n");
        wstream.write('0x' + ('00' + data[i].toString(16)).slice(-2));
        if (i<data.length-1) wstream.write(',');
    }

    wstream.write('\n};\n')
    return { 
        name: source.replace(generatedFolder,'').replace(inputFolder,'').replace(/\.gz$/g, '').replace(/\\/g, '/'),
        varName: snakeCase(path.basename(source)),
        length: data.length,
        gz: source.endsWith('.gz')
    }
};
*/

var asyncDone = true;
var eventQueue = [];
function addToQueue(f) {
  eventQueue.push(f);
}
function runNext() {
    if (!asyncDone && eventQueue.length) {
        setTimeout(() =>  runNext(), 500);
    } else if (eventQueue.length) {
        var f = eventQueue.shift();
        f();
        runNext();
    } else {
        /*
        wstream.write(`filelist_t filelist[] = {\n`);
        for (let entry of filelist) {
            wstream.write(`{ "${entry.name}", (char*)${entry.varName}, ${entry.length}, ${entry.gz?1:0}  },\n`);
        }
        wstream.write(`{ 0, 0, 0, 0 }\n};`);
        wstream.end();
        */
    }
}   

async function* getFiles(dir) {
    const items = fs.readdirSync(dir, { withFileTypes: true });
    for (const item of items) {
        let res = path.join(dir, item.name);
        if (!item.isDirectory()) {
            yield res;
        } else {
            yield* getFiles(res);
        }
    }
}
var gzip = zlib.createGzip();
async function processDir(templateDir = 'html') {
    var htmlHead = minify.html(fs.readFileSync(path.join(templateDir, 'head-')).toString(), minifyOptions);
    for await (const source of getFiles(templateDir)) { 
        //console.log(`Found ${source}`)
        /* if (/\.xhtm[l]$/.test(source)) {
            addToQueue(()=>{
                asyncDone = false;
                console.log(`Processing ${source}`);
                vulcan.process(source.replace(inputFolder,''), (_, inlineHtml) => {
                    //console.log(minifyHTML(inlineHtml));
                    //const minifiedData = minify.html(inlineHtml, minifyOptions);
                    var out = fs.createWriteStream(`${source}.gz`);
                    streamPass.write(inlineHtml);
                    streamPass.end();
                    
                    streamPass.pipe(gzip).pipe(out).on("finish", () => {
                        //convert(`${source}.gz`);
                        //gzip.end();
                        del([`${source}.gz`]);
                        console.log("")
                        asyncDone = true;
                    });
                });    
            });
        } else*/ if (/\.html$/.test(source) || /\.js$/.test(source) || /\.css$/.test(source)) {

            addToQueue(()=>{
                asyncDone = false;
                console.log(`Processing ${source}`);
                try {
                    //const outFilename = path.join(generatedFolder, source.replace(inputFolder,''))+'.gz';
                    const outFilename = path.join(generatedFolder, source.replace(inputFolder,''));
                    mkdirp.sync(path.dirname(outFilename));
                    minify(source, minifyOptions).then(data =>{
                        const o = fs.createWriteStream(outFilename);
                        const s = new stream.PassThrough();
                        //const g = zlib.createGzip();
                        s.write(/\.html$/.test(source)?htmlHead+data: data);
                        s.end();
                        s/*.pipe(g)*/.pipe(o).on("finish", () => {
                            //filelist.push(convert(outFilename));
                            //del([outFilename]);
                            //console.log(data)
                            asyncDone = true;
                        });    
                    });
    
                } catch(e) {
                    throw e;
                }

            });
        } else if (/\.jsx$/.test(source)) {
            addToQueue(()=>{
                asyncDone = false;
                console.log(`Processing ${source}`);
                minify(source, minifyOptions).then((data, err)=>{
                    if (err) {
                        throw new Error(err);
                    }
                    convert(source, data);
                    asyncDone = true;    
                });
            });
        } else if (/\.cssx$/.test(source)) {
            addToQueue(()=>{
                asyncDone = false;
                console.log(`Processing ${source}`);
                minify(source, minifyOptions).then((data, err) => {
                    if (err) {
                        throw new Error(err);
                    }
                    convert(source, data);
                    asyncDone = true;        
                });
            });
        } else {
            addToQueue(()=>{
                console.log(`Skipping ${source}`);
                //filelist.push(convert(source));
            });
        }    
    };
}
(async function() {
    await processDir(inputFolder);
    runNext();
})()

