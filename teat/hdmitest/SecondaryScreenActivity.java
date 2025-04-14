package com.liangchao.hdmitest;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.VideoView;

// SecondaryScreenActivity.java
public class SecondaryScreenActivity extends Activity {
    private static final String TAG = "SecondaryScreen";
    private int mDisplayId;
    private int mPhotoIndex;
    private ImageView mImageView;

    private VideoView mVideoView;
    private Button mButVid;
    private boolean mIsVideoPlaying = false;

    // 使用本地资源数组（可选）
    private static final int[] LOCAL_PHOTOS = new int[] {
            R.drawable.frantic,
            R.drawable.photo1, R.drawable.photo2, R.drawable.photo3,
            R.drawable.photo4, R.drawable.photo5, R.drawable.photo6,
            R.drawable.sample_4,R.drawable.timg,
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.presentation_content);

        mImageView = findViewById(R.id.image);
        Button butImg = findViewById(R.id.but_img);
        mButVid = findViewById(R.id.but_vid); // 确保ID正确
        mVideoView = findViewById(R.id.mVideoview);

        mDisplayId = getIntent().getIntExtra("displayId", -1);
        mPhotoIndex = getIntent().getIntExtra("photoIndex", 0);

        // 初始化图片
        updateImage();

        // 图片按钮点击事件
        butImg.setOnClickListener(v -> {
            mPhotoIndex = (mPhotoIndex + 1) % LOCAL_PHOTOS.length;
            updateImage();
        });

        // 视频按钮点击事件
        mButVid.setOnClickListener(v -> toggleVideoPlayback());

        // 视频初始化
        String uri = "android.resource://" + getPackageName() + "/" + R.raw.shrek;
        mVideoView.setVideoURI(Uri.parse(uri));
        mVideoView.setOnPreparedListener(mp -> Log.d(TAG, "视频准备完成"));
        mVideoView.setOnErrorListener((mp, what, extra) -> {
            Log.e(TAG, "播放错误: what=" + what + ", extra=" + extra);
            return true;
        });

    }

    private void toggleVideoPlayback() {
        if (mVideoView.isPlaying()) {
            mVideoView.pause();
            mButVid.setText("播放视频");
        } else {
            mImageView.setVisibility(View.GONE);
            mVideoView.setVisibility(View.VISIBLE);
            mVideoView.start();
            mButVid.setText("暂停视频");
        }
        mIsVideoPlaying = !mIsVideoPlaying;
    }

    private void updateImage() {
        if (mPhotoIndex >= 0 && mPhotoIndex < LOCAL_PHOTOS.length) {
            mImageView.setImageResource(LOCAL_PHOTOS[mPhotoIndex]);
        } else {
            Log.e(TAG, "Invalid photo index: " + mPhotoIndex);
        }
    }

    // 保持原有触摸事件处理逻辑
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        // 原有手势检测逻辑
        return super.onTouchEvent(event);
    }

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            int targetId = intent.getIntExtra("displayId", -1);
            if (targetId == mDisplayId) {
                finish();
            }
        }
    };

    @Override
    protected void onResume() {
        super.onResume();

        // 修复语法格式并兼容Android 13+
       // 注册广播接收器
        IntentFilter filter = new IntentFilter("CLOSE_SECONDARY_SCREEN");
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(mReceiver, filter, Context.RECEIVER_EXPORTED);
        } else {
            //registerReceiver(mReceiver, filter);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(mReceiver);
    }
}