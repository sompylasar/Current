/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2016 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

// Step 2: A small tool to visualize Fisher's Iris data same way Wikipedia did, but in clickable format.
// NOTE(dkorolev): In order to stay self-contained, further steps largely copy-paste the code of this tool.

#include "../../TypeSystem/struct.h"
#include "../../Blocks/HTTP/api.h"
#include "../../Bricks/graph/gnuplot.h"
#include "../../Bricks/file/file.h"
#include "../../Bricks/dflags/dflags.h"

#include "data/dataset.h"
using IrisFlower = Schema_Element_Object;  // Using the default type name from the autogenerated schema.

DEFINE_string(input, "data/dataset.json", "The path to the input data file.");
DEFINE_uint16(port, 3000, "The port to run the server on.");

struct Label {
  const std::string name;
  const std::string color;
  const std::string pt;  // gnuplot's "point type".
};
static std::vector<Label> labels = {
    {"setosa", "#ff0000", "7"}, {"versicolor", "#00c000", "9"}, {"virginica", "#0000c0", "11"}};

struct Feature {
  double IrisFlower::*mem_ptr;
  const std::string name;
};

static std::vector<std::pair<std::string, Feature>> features_list = {{"SL", {&IrisFlower::SL, "Sepal.Length"}},
                                                                     {"SW", {&IrisFlower::SW, "Sepal.Width"}},
                                                                     {"PL", {&IrisFlower::PL, "Petal.Length"}},
                                                                     {"PW", {&IrisFlower::PW, "Petal.Width"}}};
static std::map<std::string, Feature> features(features_list.begin(), features_list.end());

Response Plot(const std::vector<IrisFlower>& flowers,
              const std::string& x,
              const std::string& y,
              bool nolegend,
              size_t image_size,
              double point_size,
              const std::string& output_format = "svg",
              const std::string& content_type = current::net::constants::kDefaultSVGContentType) {
  try {
    const double IrisFlower::*px = features.at(x).mem_ptr;
    const double IrisFlower::*py = features.at(y).mem_ptr;
    using namespace current::gnuplot;
    GNUPlot graph;
    if (nolegend) {
      graph.NoBorder().NoTics().NoKey();
    } else {
      graph.Title("Iris Data").Grid("back").XLabel(features.at(x).name).YLabel(features.at(y).name);
    }
    graph.ImageSize(image_size).OutputFormat(output_format);
    for (const auto& label : labels) {
      auto plot_data = WithMeta([&label, &flowers, px, py](Plotter& p) {
        for (const auto& flower : flowers) {
          if (flower.Label == label.name) {
            p(flower.*px, flower.*py);
          }
        }
      });
      graph.Plot(plot_data.AsPoints()
                     .Name(label.name)
                     .Color("rgb '" + label.color + "'")
                     .Extra("pt " + label.pt + (point_size ? "ps " + current::ToString(point_size) : "")));
    }
    return Response(static_cast<std::string>(graph), HTTPResponseCode.OK, content_type);
  } catch (const std::out_of_range&) {
    return Response("Invalid dimension.", HTTPResponseCode.BadRequest);
  }
}

int main(int argc, char** argv) {
  ParseDFlags(&argc, &argv);

  const auto flowers = ParseJSON<std::vector<IrisFlower>>(current::FileSystem::ReadFileAsString(FLAGS_input));

  std::cout << "Read " << flowers.size() << " flowers." << std::endl;

  if (FLAGS_port) {
    auto& http = HTTP(FLAGS_port);
    const auto scope = http.Register(
        "/",
        [&flowers](Request r) {
          if (r.url.query.has("x") && r.url.query.has("y")) {
            r(Plot(flowers,
                   r.url.query["x"],
                   r.url.query["y"],
                   r.url.query.has("nolegend"),
                   current::FromString<size_t>(r.url.query.get("dim", "800")),
                   current::FromString<double>(r.url.query.get("ps", "1.75"))));
          } else {
            // I don't always generate HTML directly from C++. But when I do ... -- D.K.
            std::string html;
            html += "<!doctype html>\n";
            html += "<table border=1>\n";
            for (size_t y = 0; y < 4; ++y) {
              html += "  <tr>\n";
              for (size_t x = 0; x < 4; ++x) {
                if (x == y) {
                  const auto text = features_list[x].second.name;
                  html += "    <td align=center valign=center><h3><pre>" + text + "</pre></h1></td>\n";
                } else {
                  const std::string img_a = "?x=" + features_list[x].first + "&y=" + features_list[y].first;
                  const std::string img_src = img_a + "&dim=250&nolegend&ps=1";
                  html += "    <td><a href='" + img_a + "'><img src='" + img_src + "' /></a></td>\n";
                }
              }
              html += "  </tr>\n";
            }
            html += "</table>\n";
            r(html, HTTPResponseCode.OK, current::net::constants::kDefaultHTMLContentType);
          }
        });

    std::cout << "Starting the server on http://localhost:" << FLAGS_port << std::endl;

    http.Join();
  }
}
