/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Samsung Electronics
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <vector>
#include <string>
#include <string.h>
#include <QApplication>
#include "gui/mainwindow.h"
#include "gui/mainwindow.moc"
#include "wlanalyzer_base/common.h"
#include "wlanalyzer_base/analyzer.h"
#include <ev++.h>

using namespace std;
using namespace WlAnalyzer;

struct options_t
{
    options_t() : coreProtocol(""), analyze(false) {}

    string coreProtocol;
    vector<string> extensions;
    bool analyze;
    string address;
};

static void usage()
{
    fprintf(stderr, "wlanalyzer is a wayland protocol analyzer\n"
            "Usage:\twlanalyzer [OPTIONS] -- <ip address>\n\n"
            "Options:\n"
            "\t-c <file_path> - set the core protocol specification file\n"
            "\t-e <file_paths> - provide extensions of the protocol file. "
            "Use only with -c option\n"
            "\t-h - this help screen\n");
}

static int parse_cmdline(int argc, char **argv, options_t *opt)
{
    if (argc < 3)
    {
        Logger::getInstance()->log("Invalid command line argument count\n");
        usage();
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-c"))
        {
            i++;
            if (i == argc)
            {
                Logger::getInstance()->log("Core protocol specification file not specified\n");
                exit(EXIT_FAILURE);
            }

            opt->coreProtocol = argv[i];
            opt->analyze = true;
        }
        else if (!strcmp(argv[i], "-e"))
        {
            i++;
            if (i == argc)
            {
                Logger::getInstance()->log("No extensions specified\n");
                exit(EXIT_FAILURE);
            }

            while (argv[i][0] != '-')
            {
                opt->extensions.push_back(argv[i]);
                i++;

                if (i >= argc - 1)
                {
                    break;
                }
            }

            i--;
        }
        else if (!strcmp(argv[i], "--"))
        {
            i++;
            if (i == argc)
            {
                Logger::getInstance()->log("Address not specified\n");
                exit(EXIT_FAILURE);
            }
            opt->address = argv[i];
        }
        else
        {
            Logger::getInstance()->log("Unknown option %s\n", argv[i]);
            usage();
            exit(EXIT_FAILURE);
        }
    }

    if (opt->coreProtocol.empty())
    {
        Logger::getInstance()->log("No core protocol definition provided\n");
        exit(EXIT_FAILURE);
    }

    if (opt->address.empty())
    {
        Logger::getInstance()->log("No address specified\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

int main(int argc, char **argv)
{
    options_t options;
    ev::default_loop loop;

	QApplication app(argc, argv);
	MainWindow window;
	window.show();
	return app.exec();

	if (parse_cmdline(argc, argv, &options))
	{
		Logger::getInstance()->log("Invalid command line\n");
		return -1;
	}

    return 0;
}
