package com.mapbox.mapboxsdk.style.sources;

import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.UiThread;
import android.graphics.Bitmap;

import com.mapbox.mapboxsdk.style.layers.Filter;
import com.mapbox.services.commons.geojson.Feature;
import com.mapbox.services.commons.geojson.FeatureCollection;
import com.mapbox.services.commons.geojson.Geometry;
import com.mapbox.mapboxsdk.geometry.LatLngQuad;

import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;


/**
 * Image source
 *
 *  * @see <a href="https://www.mapbox.com/mapbox-gl-style-spec/#sources-image">the style specification</a>
 */
@UiThread
public class ImageSource extends Source {

  /**
   * Internal use
   *
   * @param nativePtr - pointer to native peer
   */
  public ImageSource(long nativePtr) {
    super(nativePtr);
  }

  /**
   * Create an ImageSource with the given coordinates
   *
   * @param id          The source id
   * @param coordinates The Latitude and Longitude of the four corners of the image
   */
  public ImageSource(String id, LatLngQuad coordinates) {
    initialize(id, coordinates);
  }

  /**
   * Create an ImageSource from coordinates and an image URL
   *
   * @param id          The source id
   * @param coordinates The Latitude and Longitude of the four corners of the image
   * @param url     remote json file
   */
  public ImageSource(String id, LatLngQuad coordinates, URL url) {
    initialize(id, coordinates);
    nativeSetUrl(url.toExternalForm());
  }

  /**
   * Create an ImageSource from coordinates and a bitmap image
   *
   * @param id          The source id
   * @param coordinates The Latitude and Longitude of the four corners of the image
   * @param bitmap      A Bitmap image
   */
  public ImageSource(String id, LatLngQuad coordinates, @NonNull android.graphics.Bitmap bitmap) {
    initialize(id, coordinates);
    nativeSetImage(bitmap);
  }

  /**
   * Create an ImageSource from coordinates and a resource file image
   *
   * @param id          The source id
   * @param coordinates The Latitude and Longitude of the four corners of the image
   * @param resourceId  The resource ID of a Bitmap image
   */
  public ImageSource(String id, LatLngQuad coordinates, @DrawableRes int resourceId) {
    initialize(id, coordinates);
    nativeSetImage()
  }

  /**
   * Updates the image url
   *
   * @param url An Image url
   */
  public void setUrl(URL url) {
    setUrl(url.toExternalForm());
  }

  /**
   * Updates the image url
   *
   * @param url An image url
   */
  public void setUrl(String url) {
    nativeSetUrl(url);
  }

  /**
   * Updates the image url
   *
   * @param bitmap A Bitmap image
   */
  public void setImage(@NonNull android.graphics.Bitmap bitmap) { nativeSetImage(bitmap);}

  /**
   * Updates the image url
   *
   * @param resourceId The resource ID of a Bitmap image
   */
  public void setImage(@DrawableRes int resourceId) { }

  /**
   * @return The url or null
   */
  @Nullable
  public String getUrl() {
    return nativeGetUrl();
  }


  protected native void initialize(String layerId, Object payload);
  protected native void nativeSetUrl(String url);
  protected native String nativeGetUrl();
  protected native void nativeSetImage(Bitmap bitmap);
  @Override
  protected native void finalize() throws Throwable;
}
