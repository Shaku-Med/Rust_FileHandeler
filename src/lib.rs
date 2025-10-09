use wasm_bindgen::prelude::*;
use web_sys::console;
use image::{ImageFormat, DynamicImage};
use std::io::Cursor;

#[wasm_bindgen]
extern "C" {
    #[wasm_bindgen(js_namespace = console)]
    fn log(s: &str);
}

macro_rules! console_log {
    ($($t:tt)*) => (log(&format_args!($($t)*).to_string()))
}

#[wasm_bindgen]
pub struct VideoThumbnailGenerator {
    video_data: Vec<u8>,
}

#[wasm_bindgen]
impl VideoThumbnailGenerator {
    #[wasm_bindgen(constructor)]
    pub fn new() -> VideoThumbnailGenerator {
        console_error_panic_hook::set_once();
        VideoThumbnailGenerator {
            video_data: Vec::new(),
        }
    }

    #[wasm_bindgen]
    pub fn load_video(&mut self, video_buffer: &[u8]) -> Result<(), JsValue> {
        self.video_data = video_buffer.to_vec();
        Ok(())
    }

    #[wasm_bindgen]
    pub fn generate_thumbnail(&self, time_seconds: f64, width: u32, height: u32) -> Result<Vec<u8>, JsValue> {
        if self.video_data.is_empty() {
            return Err(JsValue::from_str("No video data loaded"));
        }

        let thumbnail = self.extract_frame_at_time(time_seconds, width, height)?;
        self.encode_as_jpeg(thumbnail)
    }

    #[wasm_bindgen]
    pub fn generate_multiple_thumbnails(&self, times: &[f64], width: u32, height: u32) -> Result<Vec<Vec<u8>>, JsValue> {
        if self.video_data.is_empty() {
            return Err(JsValue::from_str("No video data loaded"));
        }

        let mut thumbnails = Vec::new();
        for &time in times {
            let thumbnail = self.extract_frame_at_time(time, width, height)?;
            let encoded = self.encode_as_jpeg(thumbnail)?;
            thumbnails.push(encoded);
        }
        Ok(thumbnails)
    }

    #[wasm_bindgen]
    pub fn get_video_info(&self) -> Result<VideoInfo, JsValue> {
        if self.video_data.is_empty() {
            return Err(JsValue::from_str("No video data loaded"));
        }

        let info = self.extract_video_info()?;
        Ok(VideoInfo::new(info.duration, info.width, info.height, info.fps))
    }
}

impl VideoThumbnailGenerator {
    fn extract_frame_at_time(&self, time_seconds: f64, width: u32, height: u32) -> Result<DynamicImage, JsValue> {
        let mut ctx = ffmpeg_next::format::input(&self.video_data)
            .map_err(|e| JsValue::from_str(&format!("Failed to open video: {}", e)))?;

        let video_stream = ctx
            .streams()
            .best(ffmpeg_next::media::Type::Video)
            .ok_or_else(|| JsValue::from_str("No video stream found"))?;

        let decoder_context = ffmpeg_next::codec::context::Context::from_parameters(video_stream.parameters())
            .map_err(|e| JsValue::from_str(&format!("Failed to create decoder: {}", e)))?;

        let mut decoder = decoder_context
            .decoder()
            .video()
            .map_err(|e| JsValue::from_str(&format!("Failed to create video decoder: {}", e)))?;

        let time_base = video_stream.time_base();
        let target_pts = (time_seconds / time_base.1 as f64 * time_base.0 as f64) as i64;

        let mut frame = ffmpeg_next::frame::Video::empty();
        let mut found_frame = false;

        for (stream, packet) in ctx.packets() {
            if stream.index() == video_stream.index() {
                decoder.send_packet(&packet)
                    .map_err(|e| JsValue::from_str(&format!("Failed to send packet: {}", e)))?;

                while decoder.receive_frame(&mut frame).is_ok() {
                    if frame.pts().unwrap_or(0) >= target_pts {
                        found_frame = true;
                        break;
                    }
                }
                if found_frame {
                    break;
                }
            }
        }

        if !found_frame {
            return Err(JsValue::from_str("Could not find frame at specified time"));
        }

        let mut scaler = ffmpeg_next::software::scaling::Context::get(
            decoder.format(),
            decoder.width(),
            decoder.height(),
            ffmpeg_next::format::Pixel::RGB24,
            width,
            height,
            ffmpeg_next::software::scaling::Flags::BILINEAR,
        ).map_err(|e| JsValue::from_str(&format!("Failed to create scaler: {}", e)))?;

        let mut scaled_frame = ffmpeg_next::frame::Video::empty();
        scaler.run(&frame, &mut scaled_frame)
            .map_err(|e| JsValue::from_str(&format!("Failed to scale frame: {}", e)))?;

        let rgb_data = scaled_frame.data(0);
        let image = image::RgbImage::from_raw(width, height, rgb_data.to_vec())
            .ok_or_else(|| JsValue::from_str("Failed to create image from frame data"))?;

        Ok(DynamicImage::ImageRgb8(image))
    }

    fn encode_as_jpeg(&self, image: DynamicImage) -> Result<Vec<u8>, JsValue> {
        let mut buffer = Vec::new();
        image.write_to(&mut Cursor::new(&mut buffer), ImageFormat::Jpeg)
            .map_err(|e| JsValue::from_str(&format!("Failed to encode JPEG: {}", e)))?;
        Ok(buffer)
    }

    fn extract_video_info(&self) -> Result<VideoMetadata, JsValue> {
        let mut ctx = ffmpeg_next::format::input(&self.video_data)
            .map_err(|e| JsValue::from_str(&format!("Failed to open video: {}", e)))?;

        let video_stream = ctx
            .streams()
            .best(ffmpeg_next::media::Type::Video)
            .ok_or_else(|| JsValue::from_str("No video stream found"))?;

        let duration = ctx.duration() as f64 / ffmpeg_next::ffi::AV_TIME_BASE as f64;
        let width = video_stream.parameters().width();
        let height = video_stream.parameters().height();
        let fps = video_stream.rate().0 as f64 / video_stream.rate().1 as f64;

        Ok(VideoMetadata {
            duration,
            width,
            height,
            fps,
        })
    }
}

struct VideoMetadata {
    duration: f64,
    width: u32,
    height: u32,
    fps: f64,
}

#[wasm_bindgen]
pub struct VideoInfo {
    duration: f64,
    width: u32,
    height: u32,
    fps: f64,
}

#[wasm_bindgen]
impl VideoInfo {
    #[wasm_bindgen(constructor)]
    pub fn new(duration: f64, width: u32, height: u32, fps: f64) -> VideoInfo {
        VideoInfo {
            duration,
            width,
            height,
            fps,
        }
    }

    #[wasm_bindgen(getter)]
    pub fn duration(&self) -> f64 {
        self.duration
    }

    #[wasm_bindgen(getter)]
    pub fn width(&self) -> u32 {
        self.width
    }

    #[wasm_bindgen(getter)]
    pub fn height(&self) -> u32 {
        self.height
    }

    #[wasm_bindgen(getter)]
    pub fn fps(&self) -> f64 {
        self.fps
    }
}

#[wasm_bindgen]
pub fn init() {
    console_error_panic_hook::set_once();
    console_log!("Video thumbnail generator initialized");
}