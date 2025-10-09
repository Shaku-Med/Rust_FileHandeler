use wasm_bindgen::prelude::*;
use wasm_bindgen::{JsCast, Clamped};
use web_sys::{window, HtmlVideoElement, HtmlCanvasElement, CanvasRenderingContext2d, Url, Blob, BlobPropertyBag};
use js_sys::{Array, Promise, Uint8Array, Function, Reflect, JsString, Object};

#[wasm_bindgen(start)]
pub fn start() {
    #[cfg(feature = "console_error_panic_hook")]
    console_error_panic_hook::set_once();
}

fn create_element<T: JsCast>(tag: &str) -> T {
    let doc = window().unwrap().document().unwrap();
    doc.create_element(tag).unwrap().dyn_into::<T>().unwrap()
}

async fn wait_event(target: &web_sys::EventTarget, event: &str) -> Result<(), JsValue> {
    let (tx, rx) = futures_channel::oneshot::channel::<()>();
    let closure = Closure::<dyn FnMut(_)>::once(move |_| {
        let _ = tx.send(());
    });
    target.add_event_listener_with_callback(event, closure.as_ref().unchecked_ref())?;
    closure.forget();
    let _ = rx.await;
    Ok(())
}

async fn load_video_from_url(url: &str) -> Result<HtmlVideoElement, JsValue> {
    let video: HtmlVideoElement = create_element("video");
    video.set_cross_origin(Some("anonymous"));
    video.set_preload("auto");
    video.set_src(url);
    video.set_muted(true);
    video.set_autoplay(false);
    video.set_loop(false);

    let target: web_sys::EventTarget = video.clone().dyn_into()?;
    let promise = Promise::new(&mut |resolve, reject| {
        let onloaded = Closure::<dyn FnMut(_)>::once(move |_| {
            let _ = resolve.call0(&JsValue::NULL);
        });
        let onerror = Closure::<dyn FnMut(_)>::once(move |_| {
            let _ = reject.call0(&JsValue::NULL);
        });
        let _ = target.add_event_listener_with_callback("loadedmetadata", onloaded.as_ref().unchecked_ref());
        let _ = target.add_event_listener_with_callback("error", onerror.as_ref().unchecked_ref());
        onloaded.forget();
        onerror.forget();
    });
    wasm_bindgen_futures::JsFuture::from(promise).await?;
    Ok(video)
}

async fn seek_video(video: &HtmlVideoElement, time: f64) -> Result<(), JsValue> {
    video.set_current_time(time);
    let target: web_sys::EventTarget = video.clone().dyn_into()?;
    let promise = Promise::new(&mut |resolve, reject| {
        let onseeked = Closure::<dyn FnMut(_)>::once(move |_| {
            let _ = resolve.call0(&JsValue::NULL);
        });
        let onerror = Closure::<dyn FnMut(_)>::once(move |_| {
            let _ = reject.call0(&JsValue::NULL);
        });
        let _ = target.add_event_listener_with_callback("seeked", onseeked.as_ref().unchecked_ref());
        let _ = target.add_event_listener_with_callback("error", onerror.as_ref().unchecked_ref());
        onseeked.forget();
        onerror.forget();
    });
    wasm_bindgen_futures::JsFuture::from(promise).await?;
    Ok(())
}

fn create_canvas(width: u32, height: u32) -> Result<(HtmlCanvasElement, CanvasRenderingContext2d), JsValue> {
    let canvas: HtmlCanvasElement = create_element("canvas");
    canvas.set_width(width);
    canvas.set_height(height);
    let ctx = canvas
        .get_context("2d")?
        .unwrap()
        .dyn_into::<CanvasRenderingContext2d>()?;
    Ok((canvas, ctx))
}

fn draw_frame_to_canvas(ctx: &CanvasRenderingContext2d, video: &HtmlVideoElement, width: u32, height: u32) -> Result<(), JsValue> {
    ctx.draw_image_with_html_video_element_and_dw_and_dh(video, 0.0, 0.0, width as f64, height as f64)?;
    Ok(())
}

fn canvas_to_png_bytes(canvas: &HtmlCanvasElement, quality: f64) -> Result<Uint8Array, JsValue> {
    let mime = JsString::from("image/png");
    let data_url = canvas.to_data_url_with_type_and_encoder_options(&mime, quality)?;
    let base64_str = data_url.split(",").nth(1).ok_or_else(|| JsValue::from_str("invalid data url"))?;
    let bytes = base64::engine::general_purpose::STANDARD.decode(base64_str).map_err(|_| JsValue::from_str("base64 decode error"))?;
    Ok(Uint8Array::from(bytes.as_slice()))
}

fn canvas_to_jpeg_bytes(canvas: &HtmlCanvasElement, quality: f64) -> Result<Uint8Array, JsValue> {
    let mime = JsString::from("image/jpeg");
    let data_url = canvas.to_data_url_with_type_and_encoder_options(&mime, quality)?;
    let base64_str = data_url.split(",").nth(1).ok_or_else(|| JsValue::from_str("invalid data url"))?;
    let bytes = base64::engine::general_purpose::STANDARD.decode(base64_str).map_err(|_| JsValue::from_str("base64 decode error"))?;
    Ok(Uint8Array::from(bytes.as_slice()))
}

#[wasm_bindgen]
pub enum ImageFormat {
    Png,
    Jpeg,
}

#[wasm_bindgen]
pub struct ThumbnailOptions {
    pub width: u32,
    pub height: u32,
    pub time: f64,
    pub quality: f64,
    pub format: ImageFormat,
}

#[wasm_bindgen]
impl ThumbnailOptions {
    #[wasm_bindgen(constructor)]
    pub fn new(width: u32, height: u32, time: f64, quality: f64, format: ImageFormat) -> ThumbnailOptions {
        ThumbnailOptions { width, height, time, quality, format }
    }
}

async fn generate_from_video(video: HtmlVideoElement, opts: &ThumbnailOptions) -> Result<Uint8Array, JsValue> {
    let width = if opts.width == 0 { video.video_width() as u32 } else { opts.width };
    let height = if opts.height == 0 { video.video_height() as u32 } else { opts.height };
    let (canvas, ctx) = create_canvas(width, height)?;
    let duration = video.duration();
    let t = if opts.time.is_finite() && opts.time >= 0.0 { opts.time.min(duration.max(0.0)) } else { 0.0 };
    seek_video(&video, t).await?;
    draw_frame_to_canvas(&ctx, &video, width, height)?;
    match opts.format {
        ImageFormat::Png => canvas_to_png_bytes(&canvas, 1.0),
        ImageFormat::Jpeg => canvas_to_jpeg_bytes(&canvas, opts.quality.max(0.0).min(1.0)),
    }
}

#[wasm_bindgen]
pub async fn thumbnail_from_url(url: String, opts: ThumbnailOptions) -> Result<Uint8Array, JsValue> {
    let video = load_video_from_url(&url).await?;
    generate_from_video(video, &opts).await
}

fn create_blob_url(bytes: &[u8], mime: &str) -> Result<String, JsValue> {
    let array = js_sys::Uint8Array::from(bytes);
    let parts = Array::new();
    parts.push(&array.buffer());
    let mut bag = BlobPropertyBag::new();
    bag.type_(mime);
    let blob = Blob::new_with_u8_array_sequence_and_options(&parts, &bag)?;
    let url = Url::create_object_url_with_blob(&blob)?;
    Ok(url)
}

#[wasm_bindgen]
pub async fn thumbnail_from_bytes(data: Uint8Array, mime: String, opts: ThumbnailOptions) -> Result<Uint8Array, JsValue> {
    let mut bytes = vec![0u8; data.length() as usize];
    data.copy_to(&mut bytes[..]);
    let url = create_blob_url(&bytes, &mime)?;
    let video = load_video_from_url(&url).await?;
    let result = generate_from_video(video, &opts).await;
    let _ = Url::revoke_object_url(&url);
    result
}

#[wasm_bindgen]
pub async fn thumbnail_data_url_from_url(url: String, opts: ThumbnailOptions) -> Result<String, JsValue> {
    let bytes = thumbnail_from_url(url, opts).await?;
    let prefix = "data:image/".to_string();
    let mime = if bytes.length() > 0 { "jpeg" } else { "png" };
    let b = js_sys::Uint8Array::new(&bytes.buffer());
    let mut vec = vec![0u8; b.length() as usize];
    b.copy_to(&mut vec[..]);
    let b64 = base64::engine::general_purpose::STANDARD.encode(vec);
    Ok(format!("{}{};base64,{}", prefix, mime, b64))
}

#[wasm_bindgen]
pub async fn thumbnail_data_url_from_bytes(data: Uint8Array, mime: String, opts: ThumbnailOptions) -> Result<String, JsValue> {
    let bytes = thumbnail_from_bytes(data, mime.clone(), opts).await?;
    let kind = if mime.contains("png") { "png" } else { "jpeg" };
    let b = js_sys::Uint8Array::new(&bytes.buffer());
    let mut vec = vec![0u8; b.length() as usize];
    b.copy_to(&mut vec[..]);
    let b64 = base64::engine::general_purpose::STANDARD.encode(vec);
    Ok(format!("data:image/{};base64,{}", kind, b64))
}
