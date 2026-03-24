package com.jetpackexample;

import androidx.core.uwb.RangingResult;
import androidx.core.uwb.RangingPosition;

import com.jetpackexample.utils.Utils;

public class RangingSample {
    private final float distanceMeters;
    private final float azimuthDegrees;
    private final float elevationDegrees;
    private final float xMeters;
    private final float yMeters;
    private final float zMeters;
    private final long timestampMillis;

    private RangingSample(float distanceMeters,
                          float azimuthDegrees,
                          float elevationDegrees,
                          float xMeters,
                          float yMeters,
                          float zMeters,
                          long timestampMillis) {
        this.distanceMeters = distanceMeters;
        this.azimuthDegrees = azimuthDegrees;
        this.elevationDegrees = elevationDegrees;
        this.xMeters = xMeters;
        this.yMeters = yMeters;
        this.zMeters = zMeters;
        this.timestampMillis = timestampMillis;
    }

    public static RangingSample fromPosition(RangingResult.RangingResultPosition rangingResultPosition) {
        RangingPosition position = rangingResultPosition.getPosition();

        if (position.getDistance() == null) {
            throw new IllegalArgumentException("Distance measurement is required");
        }

        float distanceMeters = position.getDistance().getValue();
        float azimuthDegrees = position.getAzimuth() != null ? position.getAzimuth().getValue() : 0f;
        float elevationDegrees = position.getElevation() != null ? position.getElevation().getValue() : 0f;
        float[] cartesian = Utils.sphericalToCartesian(distanceMeters, azimuthDegrees, elevationDegrees);

        return new RangingSample(
                distanceMeters,
                azimuthDegrees,
                elevationDegrees,
                cartesian[0],
                cartesian[1],
                cartesian[2],
                System.currentTimeMillis()
        );
    }

    public float getDistanceMeters() {
        return distanceMeters;
    }

    public float getAzimuthDegrees() {
        return azimuthDegrees;
    }

    public float getElevationDegrees() {
        return elevationDegrees;
    }

    public float getXMeters() {
        return xMeters;
    }

    public float getYMeters() {
        return yMeters;
    }

    public float getZMeters() {
        return zMeters;
    }

    public long getTimestampMillis() {
        return timestampMillis;
    }
}
