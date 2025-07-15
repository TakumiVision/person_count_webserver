/*
 * This file is part of [Head Count Web Application].
 *
 * Copyright (C) 2025 TakumiVision co., ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "person_counter.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

PyMODINIT_FUNC PyInit_PersonCounterModule()
{
    py::module m("PersonCounterModule", "Person counter plugin");
    // Define the Rect class
    py::class_<Rect>(m, "Rect")
        .def(py::init<>())
        .def(py::init<int, int, int, int, float>(), py::arg("x"), py::arg("y"),
             py::arg("width"), py::arg("height"), py::arg("confidence"))
        .def_readwrite("x", &Rect::x)
        .def_readwrite("y", &Rect::y)
        .def_readwrite("width", &Rect::width)
        .def_readwrite("height", &Rect::height)
        .def_readwrite("confidence", &Rect::confidence)
        .def(
            "to_dict",
            [](const Rect &rect) {
                py::dict result;
                result["x"] = rect.x;
                result["y"] = rect.y;
                result["width"] = rect.width;
                result["height"] = rect.height;
                result["confidence"] = rect.confidence;
                return result;
            },
            "Convert Rect to dictionary");
    // Define OBJPos and Thresholds classes
    py::class_<OBJPos>(m, "OBJPos")
        .def(py::init<>())
        .def(py::init<int, int>(), py::arg("x"), py::arg("y"))
        .def_readwrite("x", &OBJPos::x)
        .def_readwrite("y", &OBJPos::y)
        .def(
            "to_dict",
            [](const OBJPos &pos) {
                py::dict result;
                result["x"] = pos.x;
                result["y"] = pos.y;
                return result;
            },
            "Convert OBJPos to dictionary");
    py::class_<Thresholds>(m, "Thresholds")
        .def(py::init<float, float, float>(), py::arg("confidenceThreshold"),
             py::arg("scoreThreshold"), py::arg("nmsThreshold"))
        .def_readwrite("confidenceThreshold", &Thresholds::confidenceThreshold)
        .def_readwrite("scoreThreshold", &Thresholds::scoreThreshold)
        .def_readwrite("nmsThreshold", &Thresholds::nmsThreshold)
        .def(
            "to_dict",
            [](const Thresholds &thresholds) {
                py::dict result;
                result["confidenceThreshold"] = thresholds.confidenceThreshold;
                result["scoreThreshold"] = thresholds.scoreThreshold;
                result["nmsThreshold"] = thresholds.nmsThreshold;
                return result;
            },
            "Convert Thresholds to dictionary");

    py::class_<PersonCounter>(m, "PersonCounter")
        .def(py::init<>())
        .def("detectHeads", &PersonCounter::detectHeads, py::arg("jpegData"),
             py::arg("vertices"), py::arg("thresholds") = Thresholds(),
             "Detect heads in the given JPEG data using the specified vertices and "
             "thresholds.");
    return m.ptr();
}