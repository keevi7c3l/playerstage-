/*
 *  WebSim - Library for web-enabling and federating simulators.
 *  Copyright (C) 2009
 *    Richard Vaughan, Brian Gerkey, and Nate Koenig
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* Desc: Confederate object mananges client connections with other WebSim servers
 * Author: Richard Vaughan
 * Date: 9 March 2009
 * SVN: $Id: gazebo.h 7398 2009-03-09 07:21:49Z natepak $
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>

#include "websim.hh"
using namespace websim;

void greetingCb(evhttp_request* req, void* arg) {
	if (req == NULL) {
		puts("[websim] warning: broken connection waiting for greeting reply.");
		return;
	}

	bool* created = (bool*) arg;

	printf("cb created %p %d\n", created, *created);

	switch (req->response_code) {
	case 0:	// host not available - we just retry
		putchar('.');
		fflush(stdout);
		break;
	case 200: // OK
		(*created) = true;
		/*printf( "[websim] server responds OK to greeting with %s\n",
		 req->input_buffer->buffer );*/
		printf("[websim] server responds OK to greeting with \n");
		break;
	case 400:
		/*printf( "[websim] server responds to greeting with error (400): %s.\n",
		 req->input_buffer->buffer );*/
		printf("[websim] server responds to greeting with error (400).\n");
		break;
	default:
		/*printf( "[websim] unknown greeting response code (%u): %s.\n",
		 req->response_code,
		 req->input_buffer->buffer );*/
		printf("[websim] unknown greeting response code (%u).\n",
				req->response_code);
		break;
	}
}

WebSim::Confederate::Confederate(WebSim* ws, const std::string& uri) :
		ws(ws), puppet_list(NULL), name(uri) {

	size_t pos = uri.find(":"); // find the start of the port number
	std::string portstr = uri.substr(pos + 1); // set to the port number

	std::string hoststr = uri.substr(0, pos);
	unsigned short portnum = atoi(portstr.c_str());

	std::cout << "Conf constructing connection: " << hoststr << " : " << portstr
			<< std::endl;

	if (!(http_con = evhttp_connection_new(hoststr.c_str(), portnum)))
		printf(
				"Error: Confederate object failed to connect to server at %s:%d\n",
				hoststr.c_str(), portnum);

	char* remote_host = NULL;
	unsigned short remote_port = 0;

	// store the name and port post-connection for sanoty check
	evhttp_connection_get_peer(http_con, &remote_host, &remote_port);

	printf("\t\tConfederate %s constructed \n", name.c_str());

	// greet the server - loop until reply is received

	bool created = false;

	struct timeval last;
	last.tv_sec = 0;
	last.tv_usec = 0;

	while (!created) {
		struct evhttp_request* er = evhttp_request_new(greetingCb, &created);
		assert(er);

		struct timeval now;
		gettimeofday(&now, NULL);

		if (now.tv_sec > last.tv_sec) {
			std::string buf = "/sim/greet/dummy";

			printf("emit created %p %u\n", &created, created);
			printf("Emitting: http://%s/%s\n", name.c_str(), buf.c_str());

			int ret = evhttp_make_request(http_con, er, EVHTTP_REQ_GET,
					buf.c_str());
			if (ret != 0) {
				printf("make request returned error %d\n", ret);
				exit(0);
			}

			last.tv_sec = now.tv_sec;
		}

		// loop until something happens or a short time passes
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		event_loopexit(&tv);
		event_loop (EVLOOP_ONCE); // loops until the request has completed
	}

	ws->tick_count_expected++;
}

WebSim::Confederate::~Confederate() {
	evhttp_connection_free(http_con);
}

void WebSim::Confederate::PuppetCreationCallback(evhttp_request* req,
		void* arg) {
	Puppet* pup = (Puppet*) arg;
	if (req->response_code == 200) {
		pup->created = true;
		//printf( "Puppet %s creation ACK.\n", 
		//	  pup->name.c_str() );
	} else {
		printf("bad response regarding puppet creation \"%s\"\n",
				pup->name.c_str());
		printf("code: %d\n", req->response_code);
		//printf("response: %s\n", req->input_buffer->buffer);
		printf("response:\n");
	}
}

void WebSim::Confederate::AddPuppet(Puppet* puppet,
		const std::string& prototype) {
	struct evhttp_request* er = evhttp_request_new(PuppetCreationCallback,
			puppet);
	assert(er);

	char buf[512];
	snprintf(buf, 512, "/sim/factory/create?name=%s&type=%s",
			puppet->name.c_str(), prototype.c_str());

	//printf( "Emitting: http://%s%s\n", name.c_str(), buf );

	int ret = evhttp_make_request(http_con, er, EVHTTP_REQ_GET, buf);
	if (ret != 0) {
		printf("make request returned error %d\n", ret);
		exit(0);
	}

	// a successful response sets puppet->created to true
	while (!puppet->created)
		event_loop (EVLOOP_ONCE); // loops until the request has completed

	puppet_list.push_back(puppet);
}

void WebSim::Confederate::PuppetPushCallback(evhttp_request* req, void* arg) {
	if (req == NULL) {
		puts("[websim] warning: broken connection waiting for push reply.");
		return;
	}

	Confederate* conf = (Confederate*) arg;
	assert(conf);

	if (req->response_code == 200) {
		conf->ws->unacknowledged_pushes--;

		//printf( "conf %s puppet push ACK. Outstanding pushes %u\n",
		//	  conf->name.c_str(), conf->ws->unacknowledged_pushes );
	} else {
		printf("bad response regarding puppet push \"%s\"\n",
				conf->name.c_str());
		printf("code: %d\n", req->response_code);
		//printf("response: %s\n", req->input_buffer->buffer);
		printf("response:\n");
	}
}

int WebSim::Confederate::Push(const std::string& name, Pose p, Velocity v,
		Acceleration a) {
	//printf( "\tconfederate %s pushing state of \"%s\"\n",
	//	 this->name.c_str(),
	//	 name.c_str() );

	struct evhttp_request* er = evhttp_request_new(PuppetPushCallback, this);
	assert(er);

	char buf[512];
	snprintf(buf, 512, "/%s/pva/set?"
			"px=%.6f&py=%.6f&pz=%.6f&pr=%.6f&pp=%.6f&pa=%.6f&"
			"vx=%.6f&vy=%.6f&vz=%.6f&vr=%.6f&vp=%.6f&va=%.6f&"
			"ax=%.6f&ay=%.6f&az=%.6f&ar=%.6f&ap=%.6f&aa=%.6f", name.c_str(),
			p.x, p.y, p.z, p.r, p.p, p.a, v.x, v.y, v.z, v.r, v.p, v.a, a.x,
			a.y, a.z, a.r, a.p, a.a);

	// printf( "Emitting: http://%s%s\n", this->name.c_str(), buf );

	++ws->unacknowledged_pushes; // decremented when reply is received

	int ret = evhttp_make_request(http_con, er, EVHTTP_REQ_GET, buf);
	if (ret != 0) {
		printf("make request returned error %d\n", ret);
		exit(0);
	}

	return 0;
}

void WebSim::Confederate::TickReplyCb(evhttp_request* req, void* arg) {
	if (req == NULL) {
		puts("[websim] warning: broken connection waiting for tick reply.");
		return;
	}

	Confederate* conf = (Confederate*) arg;
	assert(conf);

	if (req->response_code == 200) {
		conf->ws->unacknowledged_ticks--;

		//printf( "tick ACK from %s. Outstanding ticks %u\n",
		//	  conf->name.c_str(),
		//	  conf->ws->unacknowledged_ticks );
	} else {
		printf("bad response regarding tick\n");
		printf("code: %d\n", req->response_code);
		//printf("response: %s\n", req->input_buffer->buffer);
		printf("response: \n");
	}
}

int WebSim::Confederate::Tick() {
	// construct and send a tick message
	//printf( "Confederate %s clock tick\n",
	//	 name.c_str() );

	struct evhttp_request* er = evhttp_request_new(TickReplyCb, (void*) this);
	assert(er);

	ws->unacknowledged_ticks++; // decremented when reply is received

	int ret = evhttp_make_request(http_con, er, EVHTTP_REQ_GET,
			"/sim/clock/tick");
	if (ret != 0) {
		printf("send tick returned error %d\n", ret);
		exit(0);
	}
	return 0;
}

