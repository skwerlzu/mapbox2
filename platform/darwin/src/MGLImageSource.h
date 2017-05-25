#import "MGLSource.h"

#import "MGLFoundation.h"
#import "MGLTypes.h"
#import "MGLGeometry.h"

NS_ASSUME_NONNULL_BEGIN

MGL_EXPORT
@interface MGLImageSource : MGLSource

#pragma mark Initializing a Source

/**
 Returns an image source with an identifier and coordinates

 @param identifier A string that uniquely identifies the source.
 @param coordinates the NW, NE, SW, and SE coordiantes for the image.
 @return An initialized image source.
 */
- (instancetype)initWithIdentifier:(NSString *)identifier coordinates:(MGLCoordinateQuad)coordinates NS_DESIGNATED_INITIALIZER;

/**
 Returns an image source with an identifier, coordinates and a URL
 
 @param identifier A string that uniquely identifies the source.
 @param coordinates the NW, NE, SW, and SE coordiantes for the image.
 @param url An HTTP(S) URL, absolute file URL, or local file URL relative to the
    current application’s resource bundle.
 @return An initialized shape source.
 */
- (instancetype)initWithIdentifier:(NSString *)identifier coordinates:(MGLCoordinateQuad)coordinates URL:(NSURL *)url;

/**
 Returns an image source with an identifier, coordinates and an image
 
 @param identifier A string that uniquely identifies the source.
 @param coordinates the NW, NE, SW, and SE coordiantes for the image.
 @param image The image to display for the sourcde.
 @return An initialized shape source.
 */
- (instancetype)initWithIdentifier:(NSString *)identifier coordinates:(MGLCoordinateQuad)coordinates image:(MGLImage *)image;

#pragma mark Accessing a Source’s Content

/**
 The URL to the source image.

 If the receiver was initialized using `-initWithIdentifier:coordinates:` or 
 `-initWithIdentifier:coordinates:Image:`, this property is set to `nil`.
 */
@property (nonatomic, copy, nullable)NSURL *URL;

@property (nonatomic, nullable, setter=setImage:)MGLImage *image;

@property (nonatomic, nonnull)CLLocationCoordinate2D * coordinates;
@end

NS_ASSUME_NONNULL_END
