/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

static String writeHeader(const String& title)
{
    return "<!DOCTYPE html><html>"
        "<head>"
        "    <style>.title{text-align:center;color:white;font-size:28pt;}.box{padding:10px;border:2px solid gray;margin:0px;background:black;color:white;-webkit-border-radius: 10px;}.box-title{text-align:center;font-weight:bold; word-break:break-all;}.true {color:green;}.false {color: red;text-decoration: line-through;}.fixed-table{color:white;border-collapse:collapse;width:100%;} tr:nth-child(2n){color:#A8A8A8;}.license{float:right;} a:link {color:#A8A8A8; text-decoration:none;}a:visited {color:#A8A8A8; text-decoration:none;}</style>"
        "    <title>"+title+"</title>"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, maximum-scale=1\">"
        "    <style>@media all and (orientation:landscape) { .main { -webkit-column-count:2; -webkit-column-rule:solid; font-size:12px; } h1 { -webkit-column-span: all; } ul { font-size: 75%; color:white } } td { text-overflow: ellipsis; overflow: hidden; }</style>"
        "    <script>"
        "        function license(credit){ "
        "        var obj = document.getElementById(credit + \"_license\");"
        "        if(obj.style.display == \"none\") { obj.style.display = \"block\"; }"
        "        else { obj.style.display = \"none\"; }}"
        "    </script>"
        "</head>"
        "<body topmargin='10'>"
        "    <div class='box'><div class='title'>"+title+"</div></div><br>"
        "    <div class='main'>";
}
