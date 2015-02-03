﻿using System;
using System.Collections.Generic;
using System.Text;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;

namespace VLC_WINRT_APP.Helpers
{
    public static class PathHelper
    {
        public static PathIcon Create(string path)
        {
            var pathIcon = new PathIcon();
            var b = new Binding
            {
                Source = path
            };
            BindingOperations.SetBinding(pathIcon, PathIcon.DataProperty, b);
            return pathIcon;
        }

        public static Path CreateGeometry(Path path, string pathGeo)
        {
            var b = new Binding
            {
                Source = pathGeo
            };
            BindingOperations.SetBinding(path, Path.DataProperty, b);
            return path;
        }
    }
}
