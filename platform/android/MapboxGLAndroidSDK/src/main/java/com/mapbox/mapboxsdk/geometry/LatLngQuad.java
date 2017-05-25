package com.mapbox.mapboxsdk.geometry;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;

import com.mapbox.mapboxsdk.exceptions.InvalidLatLngBoundsException;
import com.mapbox.services.android.telemetry.constants.GeoConstants;

import java.util.ArrayList;
import java.util.List;

/**
 * A geographical area representing a non-aligned rectangle
 * <p>
 * This class does not wrap valies to the world bounds
 * </p>
 */
public class LatLngQuad implements Parcelable {

  private final LatLng mTopLeft;
  private final LatLng mTopRight;
  private final LatLng mBottomRight;
  private final LatLng mBottomLeft;

  /**
   * Construct a new LatLngQuad based on its corners,
   * in order top left, top right, bottom left, bottom right
   */
  LatLngQuad(final LatLng tl, final LatLng tr, final LatLng br, final LatLng bl) {
    this.mTopLeft = tl;
    this.mTopRight = tr;
    this.mBottomRight = br;
    this.mBottomLeft = bl;
  }

  public LatLng getTopLeft() {
    return this.mTopLeft;
  }

  public LatLng getTopRight() {
    return this.mTopRight;
  }

  public LatLng getBottomRight() {
    return this.mBottomRight;
  }

  public LatLng getBottomLeft() {
    return this.mBottomLeft;
  }

  public static final Parcelable.Creator<LatLngQuad> CREATOR =
      new Parcelable.Creator<LatLngQuad>() {
        @Override
        public LatLngQuad createFromParcel(final Parcel in) {
          return readFromParcel(in);
        }

        @Override
        public LatLngQuad[] newArray(final int size) {
          return new LatLngQuad[size];
        }
      };

  @Override
  public int hashCode() {
    int code = mTopLeft.hashCode();
    code = (code ^ code >>> 31) + mTopRight.hashCode();
    code = (code ^ code >>> 31) + mBottomRight.hashCode();
    code = (code ^ code >>> 31) + mBottomLeft.hashCode();
    return code;
  }

  @Override
  public int describeContents() {
    return 0;
  }

  @Override
  public void writeToParcel(final Parcel out, final int arg1) {
    mTopLeft.writeToParcel(out, arg1);
    mTopRight.writeToParcel(out, arg1);
    mBottomRight.writeToParcel(out, arg1);
    mBottomLeft.writeToParcel(out, arg1);
  }

  private static LatLngQuad readFromParcel(final Parcel in) {
    final LatLng tl = new LatLng(in);
    final LatLng tr = new LatLng(in);
    final LatLng br = new LatLng(in);
    final LatLng bl = new LatLng(in);
    return new LatLngQuad(tl, tr, br, bl);
  }
}
