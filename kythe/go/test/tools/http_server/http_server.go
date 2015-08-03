/*
 * Copyright 2015 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Binary http_server exposes an HTTP interface for testing the search, xrefs,
// and filetree services backed by a combined serving table.  The server places
// the port on which it listens into the given --port_file.  Requesting
// "http://localhost:$(<"$PORT_FILE")/quitquitquit" will forcibly exit the
// server successfully.  Requesting /alive will return a 200 HTTP status once
// the server is launched.
package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"net/http"
	"os"

	"kythe.io/kythe/go/services/filetree"
	"kythe.io/kythe/go/services/search"
	"kythe.io/kythe/go/services/web"
	"kythe.io/kythe/go/services/xrefs"
	ftsrv "kythe.io/kythe/go/serving/filetree"
	srchsrv "kythe.io/kythe/go/serving/search"
	xsrv "kythe.io/kythe/go/serving/xrefs"
	"kythe.io/kythe/go/storage/leveldb"
	"kythe.io/kythe/go/storage/table"

	"golang.org/x/net/context"
)

var (
	servingTable = flag.String("serving_table", "", "LevelDB serving table")
	portFile     = flag.String("port_file", "", "File to output listening port")
)

func main() {
	flag.Parse()
	if *servingTable == "" {
		log.Fatal("Missing --serving_table argument")
	} else if *portFile == "" {
		log.Fatal("Missing --port_file argument")
	}

	db, err := leveldb.Open(*servingTable, nil)
	if err != nil {
		log.Fatalf("Error opening db at %q: %v", *servingTable, err)
	}
	defer db.Close()
	tbl := &table.KVProto{db}
	xs := xsrv.NewCombinedTable(tbl)
	ft := &ftsrv.Table{tbl}
	sr := &srchsrv.Table{&table.KVInverted{db}}

	ctx := context.Background()
	xrefs.RegisterHTTPHandlers(ctx, xs, http.DefaultServeMux)
	filetree.RegisterHTTPHandlers(ctx, ft, http.DefaultServeMux)
	search.RegisterHTTPHandlers(ctx, sr, http.DefaultServeMux)
	web.RegisterQuitHandler(http.DefaultServeMux)
	http.HandleFunc("/alive", func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprintln(w, "ok")
	})

	l, err := net.Listen("tcp", "localhost:0")
	if err != nil {
		log.Fatal(err)
	}

	_, port, err := net.SplitHostPort(l.Addr().String())
	if err != nil {
		log.Fatal(err)
	}

	if err := ioutil.WriteFile(*portFile, []byte(port+"\n"), 0777); err != nil {
		log.Fatal(err)
	}
	defer os.Remove(*portFile) // ignore errors
	log.Fatal(http.Serve(l, nil))
}
