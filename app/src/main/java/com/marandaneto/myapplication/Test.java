package com.marandaneto.myapplication;

import io.sentry.Sentry;

public class Test {
    public static void capture(String value) {
        Sentry.capture(value);
    }
}
