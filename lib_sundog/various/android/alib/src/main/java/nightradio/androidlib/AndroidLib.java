package nightradio.androidlib;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.net.Uri;
import android.opengl.GLES20;
import android.os.Bundle;
import android.os.Environment;
import android.os.PowerManager;
import android.util.Log;

import java.io.File; 
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream; 
import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.util.List;
import java.util.Locale;
import java.util.zip.ZipEntry; 
import java.util.zip.ZipInputStream;

import com.intel.inde.mp.*;
import com.intel.inde.mp.android.AndroidMediaObjectFactory;
import com.intel.inde.mp.android.VideoFormatAndroid;
import com.intel.inde.mp.android.graphics.EglUtil;
import com.intel.inde.mp.android.graphics.FrameBuffer;
import com.intel.inde.mp.android.graphics.FullFrameTexture;
import com.intel.inde.mp.domain.Resolution;

import com.coremedia.iso.boxes.Container;
import com.googlecode.mp4parser.authoring.Movie;
import com.googlecode.mp4parser.authoring.Track;
import com.googlecode.mp4parser.authoring.builder.DefaultMp4Builder;
import com.googlecode.mp4parser.authoring.container.mp4.MovieCreator;

public class AndroidLib 
{
	static 
	{
        System.loadLibrary( "sundog" );
    }
	
	public static native int camera_frame_callback( byte[] data, Camera camera );
	
	private Activity activity;
	private Boolean wlInit;
	private PowerManager pm;
	private PowerManager.WakeLock wl;
	private Camera cam;
	private int camWidth;
	private int camHeight;
	private int camTexture;
	private VideoFormat videoFormat;
    private GLCapture GLCapturer;
    private FrameBuffer GLCapFrameBuffer;
    private FullFrameTexture GLCapTexture;
    private String GLCapFileName;
    private VideoFormatAndroid GLCapVideoFormat;
    private volatile Boolean GLCapStarted = false;
    private volatile Boolean GLCapFinished = false;
    private int GLCapWidth = 0;
    private int GLCapHeight = 0;
    private int GLCapFPS = 0;
	private long GLCapNextCaptureNanoTime = 0;
	private long GLCapStartNanoTime = 0;
	private int GLCapFramesReceived = 0;
	private int GLCapFramesCaptured = 0;
	private BufferedInputStream mZipInputStream; 
	private String mLocation;
	private byte[] internalHash = new byte[ 128 ];
	private int internalHashLen;
	
	public AndroidLib( Activity act )
	{
		activity = act;
		wlInit = false;
		cam = null;
		camTexture = 256;
	}
	
	public String GetDir( String dirID ) 
    {
    	if( dirID.equals( "internal_cache" ) )
    	{
    		try { 
    			File f = activity.getApplicationContext().getCacheDir();
    			return f.toString();
    		}
    		catch( Exception e ) 
    		{
    			Log.e( "GetDir( internal_cache )", "getCacheDir() error", e ); 
    			return null;
    		}
    	}
    	if( dirID.equals( "internal_files" ) )
    	{
    		try { 
        		File f = activity.getApplicationContext().getFilesDir();    	
        		return f.toString();
        	}
        	catch( Exception e ) 
        	{
        		Log.e( "GetDir( internal_files )", "getFilesDir() error", e ); 
        		return null;
        	}
    	}
    	if( dirID.equals( "external_cache" ) )
    	{
    		try {
        		File f = activity.getApplicationContext().getExternalCacheDir();    	
        		return f.toString();
        	}
        	catch( Exception e ) 
        	{
        		Log.e( "GetDir( external_cache )", "getFilesDir() error", e ); 
        		return null;
        	}
    	}
    	if( dirID.equals( "external_files" ) )
    	{
    		try {
        		File f = activity.getApplicationContext().getExternalFilesDir( null );    	
        		return f.toString();
        	}
        	catch( Exception e ) 
        	{
        		Log.e( "GetDir( external_files )", "getExternalFilesDir() error", e ); 
        		return null;
        	}
    	}
    	if( dirID.equals( "external_dcim" ) )
    	{
    		try {
        		File f = Environment.getExternalStorageDirectory();  
        		File dcim = new File( f.getAbsolutePath() + "/" + Environment.DIRECTORY_DCIM );
        		return dcim.toString();
        	}
        	catch( Exception e ) 
        	{
        		Log.e( "GetDir( external_dcim )", "getExternalFilesDir() error", e ); 
        		return null;
        	}
    	}
    	if( dirID.equals( "external_pictures" ) )
    	{
    		try {
        		File f = Environment.getExternalStorageDirectory();  
        		File dcim = new File( f.getAbsolutePath() + "/" + Environment.DIRECTORY_PICTURES );
        		return dcim.toString();
        	}
        	catch( Exception e ) 
        	{
        		Log.e( "GetDir( external_pictures )", "getExternalFilesDir() error", e ); 
        		return null;
        	}
    	}
    	if( dirID.equals( "external_movies" ) )
    	{
    		try {
        		File f = Environment.getExternalStorageDirectory();  
        		File dcim = new File( f.getAbsolutePath() + "/" + Environment.DIRECTORY_MOVIES );
        		return dcim.toString();
        	}
        	catch( Exception e ) 
        	{
        		Log.e( "GetDir( external_movies )", "getExternalFilesDir() error", e ); 
        		return null;
        	}
    	}
    	return null;
    }
    
    public String GetOSVersion()
    {
    	return android.os.Build.VERSION.RELEASE;
    }
    
    public String GetLanguage()
    {
    	return Locale.getDefault().toString();
    }

	boolean FileExists( String name )
	{
		File f = new File( name );
		if( f.exists() )
			return true;
		else
			return false;
	}

    public String GetIntentFile() //Get the file name from some another app
    {
    	String rv = null;
    	Intent intent = activity.getIntent();
        String action = intent.getAction();
        if( action.compareTo( Intent.ACTION_VIEW ) == 0 ||
        	action.compareTo( Intent.ACTION_SEND ) == 0 ||
        	action.compareTo( Intent.ACTION_EDIT ) == 0 ) 
        {
            ContentResolver resolver = activity.getApplicationContext().getContentResolver();
            Uri uri;
            String name;
            if( action.compareTo( Intent.ACTION_SEND ) == 0 )
            {
            	Bundle bundle = intent.getExtras();
            	uri = (Uri)bundle.get( Intent.EXTRA_STREAM );
            	name = "shared file";
            }
            else
            {
            	uri = intent.getData();
                name = uri.getLastPathSegment();
            }
            if( name == null ) name = "shared file";
            rv = GetDir( "external_files" ) + "/" + name;
			if( FileExists( rv ) ) rv = rv + "_temp";
            Log.v( "GetIntentFile" , "File intent detected: " + action + " : " + intent.getDataString() + " : " + intent.getType() + " : " + rv );
            try {
            	InputStream input = resolver.openInputStream( uri );
                OutputStream out = new FileOutputStream( new File( rv ) );
                int size = 0;
                byte[] buffer = new byte[ 1024 ];
                while( ( size = input.read( buffer ) ) != -1 ) 
                {
                    out.write( buffer, 0, size );
                }
                out.close();
                input.close();
            }
            catch( Exception e ) 
            {
                Log.e( "GetIntentFile", "InputStreamToFile exception: " + e.getMessage() );
                rv = null;
            }
        }
        return rv;
    }

    public int OpenURL( String s )
    {
    	Intent i = new Intent( Intent.ACTION_VIEW );  
    	i.setData( Uri.parse( s ) );  
    	activity.startActivity( i );
    	return 0;
    }
    
    public int ScanMedia( String path ) 
    {
        File file = new File( path );
        Uri uri = Uri.fromFile( file );
        Intent scanFileIntent = new Intent( Intent.ACTION_MEDIA_SCANNER_SCAN_FILE, uri );
        activity.sendBroadcast( scanFileIntent );
        return 0;
    }
    
    public int SetCameraTexture( int tex )
    {
    	camTexture = tex;
    	Log.i( "Camera", "Camera texture: " + tex );
    	return 0;
    }
    
    private void CloseCameraAndWait()
    {
    	CloseCamera();
		try {
			Thread.sleep( 100 );
		} catch( Exception ee ) { 
			Log.e( "mAllow Sleep", "Exception", ee ); 
		}
    }
    
    public int OpenCamera( int camId )
    {
    	int attempts = 4;
    	for( int a = attempts; a > 0; a-- )
    	{
    		Log.i( "Camera", "Open attempt " + ( attempts - a + 1 ) );
    		if( cam != null )
    		{
    			//Already opened:
    			return 0;
    		}
    		try 
    		{
    			cam = Camera.open();
    		}
    		catch( Exception e )
    		{
    			//Camera is not available (in use or does not exist)
    			Log.e( "Camera.open()", e.getMessage() );
    			if( a <= 1 )
    				return -1;
    			else
    			{
    				CloseCameraAndWait();
    				continue;
    			}
    		}
    		Camera.Parameters pars = cam.getParameters();
    		Log.i( "Camera", "Got parameters" );
    		try 
    		{
    			cam.setParameters( pars );
    		}
    		catch( Exception e )
    		{
    			Log.e( "Camera.setParameters()", e.getMessage() );
    			if( a <= 1 )
    				return -2;
    			else
    			{
    				CloseCameraAndWait();
    				continue;
    			}
    		}
    		Log.i( "Camera", "New parameters saved" );
    		Size previewSize = pars.getPreviewSize();
    		camWidth = previewSize.width;
    		camHeight = previewSize.height;
    		int frame_size = camWidth * camHeight;
    		cam.addCallbackBuffer( new byte[ frame_size + frame_size / 2 ] );
    		cam.addCallbackBuffer( new byte[ frame_size + frame_size / 2 ] );
    		cam.setPreviewCallbackWithBuffer( new Camera.PreviewCallback() {
    			public synchronized void onPreviewFrame( byte[] data, Camera camera ) 
    			{
    				camera_frame_callback( data, camera );
    				camera.addCallbackBuffer( data );
    				return;
    			}
    		} );
    		try 
    		{
    			if( android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.HONEYCOMB && camTexture != 0 ) 
    			{
    				cam.setPreviewTexture( new SurfaceTexture( camTexture ) );
    			}
    			else
    			{
    				cam.setPreviewDisplay( null ); //Link to the empty surface
    			}
    		}
    		catch( Exception e )
    		{
    			Log.e( "Camera.setPreviewDisplay()", e.getMessage() );
    			if( a <= 1 )
    				return -3;
    			else
    			{
    				CloseCameraAndWait();
    				continue;
    			}
    		}    	
    		Log.i( "Camera", "Starting preview..." );
    		try 
    		{
    			cam.startPreview();
    		}
    		catch( Exception e )
    		{
    			Log.e( "Camera.startPreview()", e.getMessage() );
    			if( a <= 1 )
    				return -4;
    			else
    			{
    				CloseCameraAndWait();
    				continue;
    			}
    		}
    		break; //successful
    	}
    	Log.i( "Camera", "Camera opened" );
    	return 0;
    }

    public int CloseCamera()
    {
    	if( cam != null )
    	{
    		cam.release();
    		cam = null;
        	Log.i( "Camera", "Camera closed" );
    	}
    	return 0;    	
    }
    
    public int GetCameraWidth()
    {
    	return camWidth;
    }

    public int GetCameraHeight()
    {
    	return camHeight;    	
    }
    
    public int GetCameraFocusMode()
    {
    	int rv = 0;
    	Camera.Parameters pars = cam.getParameters();
    	String mode = pars.getFocusMode();
    	if( mode == Camera.Parameters.FOCUS_MODE_AUTO ) rv = 0;
    	if( mode == Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO ) rv = 1;
    	if( mode == Camera.Parameters.FOCUS_MODE_FIXED ) rv = 2;
    	if( mode == Camera.Parameters.FOCUS_MODE_INFINITY ) rv = 3;
    	return rv;    	
    }
    
    public int SetCameraFocusMode( int mode_num )
    {
    	Camera.Parameters pars = cam.getParameters();
    	String mode = null;
    	switch( mode_num )
    	{
    		case 0: mode = Camera.Parameters.FOCUS_MODE_AUTO; break;
    		case 1: mode = Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO; break;
    		case 2: mode = Camera.Parameters.FOCUS_MODE_FIXED; break;
    		case 3: mode = Camera.Parameters.FOCUS_MODE_INFINITY; break;
    	}
    	if( mode != null )
    	{
    		List<String> focusModes = pars.getSupportedFocusModes();
    		if( focusModes.contains( mode ) ) 
    		{
    			pars.setFocusMode( mode );
        		cam.setParameters( pars );
            	if( mode_num == 0 )
            	{
            		cam.autoFocus( null );
            	}
    		}
    	}
    	return 0;
    }
    
    private IProgressListener GLCapProgressListener = new IProgressListener() 
    {
    	@Override
    	public void onMediaStart() 
    	{ 
    		Log.i( "GLCapture progress", "Start" );
    		GLCapStarted = true;
    	}

    	@Override
    	public void onMediaProgress( float progress ) { }
    	
    	@Override
    	public void onMediaDone() 
    	{ 
    		Log.i( "GLCapture progress", "Done" ); 
    		GLCapFinished = true; 
    	}

    	@Override
    	public void onMediaPause() { }
    	
    	@Override
    	public void onMediaStop() 
    	{ 
    		Log.i( "GLCapture progress", "Stop" ); 
    	}

    	@Override
    	public void onError( Exception exception ) 
    	{ 
    		Log.e( "GLCapture progress", "error", exception );
    		GLCapFinished = true;
    	}
    };
       
    public int GLCaptureStart( int width, int height, int fps, int bitrate_kb )
    {
    	int rv = -1;
    	
    	if( GLCapturer != null ) return -1;
    	GLCapFinished = false;
    	GLCapStarted = false;
    	GLCapFramesCaptured = 0;
    	GLCapFramesReceived = 0;
    	
    	Log.i( "GLCaptureStart", width + " " + height + " " + fps + " " + bitrate_kb );
    	GLCapWidth = width;
    	GLCapHeight = height;
    	GLCapFPS = fps;
    	while( true )
    	{
    		GLCapturer = new GLCapture( new AndroidMediaObjectFactory( activity ), GLCapProgressListener );
    		if( GLCapturer != null )
    		{
    			Log.i( "GLCaptureStart", "new GLCapture ok" );
    		}
    		else
    		{
    			break;
    		}
    	
    		GLCapVideoFormat = new VideoFormatAndroid( "video/avc", width, height ); 
    		GLCapVideoFormat.setVideoBitRateInKBytes( bitrate_kb );       
    		GLCapVideoFormat.setVideoFrameRate( fps );       
    		GLCapVideoFormat.setVideoIFrameInterval( 1 );
    		GLCapturer.setTargetVideoFormat( GLCapVideoFormat );
    	
    		String externalFilesDir = GetDir( "external_files" );
            if( externalFilesDir == null )
            {
            	Log.e( "GLCaptureStart", "Can't get external files dir" );
                break;
            }
            GLCapFileName = externalFilesDir + "/glcapture.mp4";
    		try {
				GLCapturer.setTargetFile( GLCapFileName );
			} catch( IOException e ) 
			{
				Log.e( "GLCaptureStart", "setTargetFile() error", e );
				break;
			}
    		
    		GLCapturer.start();    		
    		while( GLCapStarted == false )
			{
				try {
					Thread.sleep( 10 );
				} catch( Exception e ) { 
					Log.e( "GLCaptureEncode", "Thread.sleep() error", e ); 
				}
			}
    		GLCapStartNanoTime = System.nanoTime();
    		GLCapNextCaptureNanoTime = 0;
   	    
        	try {
        		GLCapturer.setSurfaceSize( width, height );
        	} catch( Exception e ) {
        		Log.e( "GLCaptureStart", "setSurfaceSize() error", e );
        		break;
        	}
        	
    		GLCapFrameBuffer = new FrameBuffer( EglUtil.getInstance() );
    	    GLCapFrameBuffer.setResolution( new Resolution( width, height ) );
    	    GLCapTexture = new FullFrameTexture();

    		rv = 0;
    		break;
    	}
    	
    	if( rv < 0 )
    	{
    		GLCaptureStop();
    	}
    	
    	return rv;
    }
    
    public int GLCaptureFrameBegin()
    {
    	if( GLCapturer == null ) return -1;
    	GLCapFrameBuffer.bind();
    	return 0;
    }

    private int GLCaptureFrame( int textureID )
    {
    	if( GLCapturer == null ) return -1;
        synchronized( GLCapturer ) 
        {
        	GLCapturer.beginCaptureFrame();
            //GLES20.glViewport( 0, 0, GLCapWidth, GLCapHeight );
            GLCapTexture.draw( textureID );
            GLCapturer.endCaptureFrame();
            GLCapFramesCaptured++;
        }
        return 0;
    }
    
    public int GLCaptureFrameEnd()
    {
    	if( GLCapturer == null ) return -1;
    	GLCapFrameBuffer.unbind();
    	int textureID = GLCapFrameBuffer.getTextureId();
		long elapsedNanoTime = System.nanoTime() - GLCapStartNanoTime;
		if( elapsedNanoTime > GLCapNextCaptureNanoTime ) 
		{
			GLCaptureFrame( textureID );
			GLCapNextCaptureNanoTime += 1000000000 / GLCapFPS;
		}
		//GLES20.glViewport( 0, 0, GLCapWidth, GLCapHeight );
		GLES20.glDisable( GLES20.GL_BLEND );
		GLCapTexture.draw( textureID );
		GLES20.glEnable( GLES20.GL_BLEND );
		GLCapFramesReceived++;
    	return 0;
    }

    public int GLCaptureStop()
    {
    	if( GLCapturer == null ) return -1;
    	GLCapturer.stop();
    	while( GLCapFinished == false )
		{
			try {
				Thread.sleep( 50 );
			} catch( Exception e ) { 
				Log.e( "GLCaptureStop", "Thread.sleep() error", e ); 
			}
		}
    	GLCapturer = null;
    	Log.i( "GLCaptureStop", "Frames received:" + GLCapFramesReceived + " captured:" + GLCapFramesCaptured );
    	return 0;
    }

    public int GLCaptureEncode( String destFileName )
    {
    	int rv = -1;
    	
    	String externalFilesDir = GetDir( "external_files" );
    	
    	String audioFileName = externalFilesDir + "/glcapture.wav";
    	File audioFile = new File( audioFileName );
    	/*int ch = 2;
    	int sfreq = 44100;
    	try {
    		FileInputStream f = new FileInputStream( audioFile );
    		byte[] wavHeader = new byte[ 36 ];
			f.read( wavHeader );            
			ch = wavHeader[ 22 ];
			sfreq = ( wavHeader[ 24 ] & 255 ) + ( ( wavHeader[ 25 ] & 255 ) << 8 ) + ( ( wavHeader[ 26 ] & 255 ) << 16 ) + ( ( wavHeader[ 27 ] & 255 ) << 24 );
			Log.i( "GLCaptureEncode", "WAV Header: " + sfreq + " " + ch );
			f.close();
    	} catch( FileNotFoundException e ) {
			Log.e( "GLCaptureEncode", "File not found!", e );
    	} catch( IOException e ) {
    		Log.e( "GLCaptureEncode", "IO exception!", e );
    	}*/
    	
    	if( audioFile.exists() )
    	{
	    	String aacFileName = externalFilesDir + "/glcapture.aac";
			File aacFile = new File( aacFileName );
			if( aacFile.exists() ) aacFile.delete();

			try {
    			FileInputStream fis = new FileInputStream( audioFile );
    			//FileOutputStream fos = new FileOutputStream( aacFile );
            
    			byte[] wavHeader = new byte[ 36 ];
    			fis.read( wavHeader );            
    			int ch = wavHeader[ 22 ];
    			int sfreq = ( wavHeader[ 24 ] & 255 ) + ( ( wavHeader[ 25 ] & 255 ) << 8 ) + ( ( wavHeader[ 26 ] & 255 ) << 16 ) + ( ( wavHeader[ 27 ] & 255 ) << 24 );
    			Log.i( "GLCaptureEncode", "WAV Header: " + sfreq + " " + ch );

    			fis.read( wavHeader, 0, 8 );
    			int dataSize = ( wavHeader[ 4 ] & 255 ) + ( ( wavHeader[ 5 ] & 255 ) << 8 ) + ( ( wavHeader[ 6 ] & 255 ) << 16 ) + ( ( wavHeader[ 7 ] & 255 ) << 24 );
    			Log.i( "GLCaptureEncode", "WAV Data size: " + dataSize );

    			MediaMuxer mux = new MediaMuxer( aacFile.getAbsolutePath(), MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4 );
    			
    			MediaFormat outputFormat = MediaFormat.createAudioFormat( "audio/mp4a-latm", sfreq, ch );
    			outputFormat.setInteger( MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC );
    			outputFormat.setInteger( MediaFormat.KEY_BIT_RATE, 128000 );
            
    			MediaCodec codec = MediaCodec.createEncoderByType( "audio/mp4a-latm" );
    			codec.configure( outputFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE );
    			codec.start();
            
    			ByteBuffer[] codecInputBuffers = codec.getInputBuffers();
    			ByteBuffer[] codecOutputBuffers = codec.getOutputBuffers();

    			MediaCodec.BufferInfo outBuffInfo = new MediaCodec.BufferInfo();

    			byte[] tempBuffer = new byte[ sfreq * ch * 2 ];
    			boolean hasMoreData = true;
    			double presentationTimeUs = 0;
    			int totalBytesRead = 0;
    			int percentComplete;
    			int audioTrackIdx = 0;
            
    			do {
    				int inputBufIndex = 0;
    				while( inputBufIndex != -1 && hasMoreData ) 
    				{
    					inputBufIndex = codec.dequeueInputBuffer( 5000 );
    					if( inputBufIndex >= 0 ) 
    					{
    						ByteBuffer dstBuf = codecInputBuffers[ inputBufIndex ];
    						dstBuf.clear();
    						int	bytesRead = fis.read( tempBuffer, 0, dstBuf.limit() );
    						if( bytesRead == -1 ) 
    						{ // -1 implies EOS
    							hasMoreData = false;
    							codec.queueInputBuffer( inputBufIndex, 0, 0, (long)presentationTimeUs, MediaCodec.BUFFER_FLAG_END_OF_STREAM );
    						} else {
    							totalBytesRead += bytesRead;
    							dstBuf.put( tempBuffer, 0, bytesRead );
    							codec.queueInputBuffer( inputBufIndex, 0, bytesRead, (long)presentationTimeUs, 0 );
    							presentationTimeUs = 1000000l * ( totalBytesRead / ( 2 * ch ) ) / sfreq;
    						}
    					}
    				}
    				// Drain audio
    				int outputBufIndex = 0;
    				while( outputBufIndex != MediaCodec.INFO_TRY_AGAIN_LATER ) 
    				{
    					outputBufIndex = codec.dequeueOutputBuffer( outBuffInfo, 5000 );
    					if( outputBufIndex >= 0 ) 
    					{
    						ByteBuffer encodedData = codecOutputBuffers[ outputBufIndex ];
    						encodedData.position( outBuffInfo.offset );
    						encodedData.limit( outBuffInfo.offset + outBuffInfo.size );
    						if( ( outBuffInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG ) != 0 && outBuffInfo.size != 0 ) 
    						{
    							codec.releaseOutputBuffer( outputBufIndex, false );
    						} 
    						else 
    						{
    							//encodedData.get( tempBuffer, outBuffInfo.offset, outBuffInfo.size );
    							//fos.write( tempBuffer, 0, outBuffInfo.size );
    							mux.writeSampleData( audioTrackIdx, codecOutputBuffers[ outputBufIndex ], outBuffInfo );
    							codec.releaseOutputBuffer( outputBufIndex, false );
    						}
    					} else if ( outputBufIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED ) {
    						outputFormat = codec.getOutputFormat();
    	                    Log.v( "GLCaptureEncode", "Output format changed - " + outputFormat );
    	                    audioTrackIdx = mux.addTrack( outputFormat );
    	                    mux.start();
    					} else if( outputBufIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED ) {
    						Log.e( "GLCaptureEncode", "Output buffers changed during encode!" );
    					} else if( outputBufIndex == MediaCodec.INFO_TRY_AGAIN_LATER ) {
    					} else {
    						Log.e( "GLCaptureEncode", "Unknown return code from dequeueOutputBuffer - " + outputBufIndex );
    					}
    				}
    				percentComplete = (int) Math.round( ( (float)totalBytesRead / (float)dataSize ) * 100.0 );
    				Log.v( "GLCaptureEncode", "Conversion % - " + percentComplete );
    			} while( outBuffInfo.flags != MediaCodec.BUFFER_FLAG_END_OF_STREAM );

    			fis.close();
    			//fos.close();
    			mux.stop();
    	        mux.release();
    			codec.stop();
    			codec.release();
    			
    			audioFile.delete(); //Delete source WAV
    			rv = 0;
    		} catch( FileNotFoundException e ) {
    			Log.e( "GLCaptureEncode", "File not found!", e );
    		} catch( IOException e ) {
    			Log.e( "GLCaptureEncode", "IO exception!", e );
    		}
    		
    		if( rv == 0 )
    		{
    			try {
        			Movie inputVideo = MovieCreator.build( GLCapFileName );
        			Movie inputAudio = MovieCreator.build( aacFileName );
        			Track videoTrack = null;
        			Track audioTrack = null;
	    			for( Track t : inputVideo.getTracks() ) 
	    			{
	    				if( t.getHandler().equals( "vide" ) ) 
	    				{
	    					videoTrack = t;
	    					break;
	    				}
	    			}
	    			for( Track t : inputAudio.getTracks() ) 
	    	    	{
	    	    		if( t.getHandler().equals( "soun" ) ) 
	    	    		{
	    	    			audioTrack = t;
	    	    			break;
	    	    		}
	    	    	}
	    			if( videoTrack != null && audioTrack != null )
	    			{
	    	    		Movie result = new Movie();
	    					
	    	    		result.addTrack( videoTrack );
	    	    		result.addTrack( audioTrack );
	    					
	    	    		Container out = new DefaultMp4Builder().build( result );
	    	    		RandomAccessFile f = new RandomAccessFile( String.format( destFileName ), "rw" );
	    	    		FileChannel fc = f.getChannel();
	    	    		out.writeContainer( fc );
	    	    		fc.close();
	    	    		f.close();
	    	    					
	    	    		//Successful!
	    	    		File sourceVideoFile = new File( GLCapFileName );
	    	    		sourceVideoFile.delete(); //Delete MP4 source
	    	    		aacFile.delete(); //Delete AAC
	    	    	}
				} catch( IOException e ) {
					Log.e( "GLCaptureEncode", "Combining AAC with MP4", e );
				}
    		}
    	}
    	else
    	{
    		//No audio:
    	}
    	
    	/*GLCapFinished = false;
    	GLCapStarted = false;
    	
    	if( audioFile.exists() )
    	{
    		while( true )
    		{
    			MediaComposer mediaComposer = new MediaComposer( new AndroidMediaObjectFactory( activity ), GLCapProgressListener );
    		
    			try {
    				mediaComposer.addSourceFile( GLCapFileName );
    			} catch( IOException e ) 
    			{
    				Log.e( "GLCaptureEncode", "addSourceFile() error", e );
    				break;
    			}
    			try {
    				mediaComposer.setTargetFile( destFileName );
    			} catch( IOException e ) 
    			{
    				Log.e( "GLCaptureEncode", "setTargetFile() error", e );
    				break;
    			}
    		
    			mediaComposer.setTargetVideoFormat( GLCapVideoFormat );
    		
    			AudioFormatAndroid audioFormat = new AudioFormatAndroid( "audio/mp4a-latm", sfreq, ch );    		
    			audioFormat.setAudioProfile( MediaCodecInfo.CodecProfileLevel.AACObjectLC );
    			audioFormat.setKeyMaxInputSize( 44 * 1024 );
    			audioFormat.setAudioBitrateInBytes( ( 128 * 1024 ) / 8 );
    			mediaComposer.setTargetAudioFormat( audioFormat );
    		
    			AudioFormat audioFormat2 = new AudioFormatAndroid( "audio/mp4a-latm", sfreq, ch );
    		
    			SubstituteAudioEffect effect = new SubstituteAudioEffect();
    			com.intel.inde.mp.Uri output = new com.intel.inde.mp.Uri( audioFileName );
    			effect.setFileUri( activity.getApplicationContext(), output, audioFormat2 );
    			mediaComposer.addAudioEffect( effect );
    		
    			mediaComposer.start(); 
    			while( GLCapFinished == false )
    			{
    				try {
    					Thread.sleep( 100 );
    				} catch( Exception e ) { 
    					Log.e( "GLCaptureEncode", "Thread.sleep() error", e ); 
    				}
    			}
    			rv = 0;
    			break;
    		}
    	}
    	else
    	{
    		//No audio:
    	}*/
    	
    	return rv;
    }
    
	private void dirChecker( String dir ) 
	{ 
	    File f = new File( mLocation + dir ); 
	 
	    if( !f.isDirectory() ) 
	    { 
	    	f.mkdirs(); 
	    } 
	} 
	
	public void UnZip( BufferedInputStream zipInputStream, String location ) 
	{
		mZipInputStream = zipInputStream; 
	    mLocation = location; 
	 
	    dirChecker( "" );
	    
		try { 
			BufferedInputStream fin = mZipInputStream; 
			ZipInputStream zin = new ZipInputStream( fin ); 
			ZipEntry ze = null; 
			while( ( ze = zin.getNextEntry() ) != null ) 
			{ 
				Log.i( "Decompress", "Unzipping " + ze.getName() ); 
	 
				if( ze.isDirectory() ) 
				{ 
					dirChecker( ze.getName() ); 
				} 
				else 
				{
					FileOutputStream fout = new FileOutputStream( mLocation + ze.getName() );
					byte[] buffer = new byte[ 4096 ];
					int length;
					while( ( length = zin.read( buffer ) ) > 0 ) 
					{
						fout.write( buffer, 0, length );
					}					
					zin.closeEntry(); 
					fout.close();
				}
			} 
			zin.close(); 
	    } catch( Exception e ) { 
	    	Log.e( "Decompress", "unzip", e ); 
	    }
	}
	
	public int CheckAppResources( InputStream internalHashStream )
	{
    	int changes = 0;
    	int len2 = 0;
    	int noExternalHash = 0;
    	BufferedInputStream h1 = null;
    	FileInputStream h2 = null;
    	
    	//Read internal hash:
    	h1 = new BufferedInputStream( internalHashStream );
    	try { 
    		internalHashLen = h1.read( internalHash );
    		h1.close();
    	} catch( Exception e ) { 
	    	Log.e( "CopyResources", "Open hash (local)", e ); 
	    	return -1;
	    }
    	
    	//Try to open external hash:
    	String externalFilesDir = GetDir( "external_files" );
    	if( externalFilesDir == null )
    	{
    		Log.e( "CopyResources", "Can't get external files dir" );
    		return -2;
    	}
    	try {
    		h2 = new FileInputStream( externalFilesDir + "/hash" );
    	} catch( FileNotFoundException e ) {
    		Log.i( "CopyResources", "Hash not found on external storage" );
    		noExternalHash = 1;
    		changes = 1;
    	}
    	
    	if( noExternalHash == 0 )
    	{
    		//Compare internal and external hashes:
    		try {
    			byte[] buf2 = new byte[ 128 ];
    			len2 = h2.read( buf2 );
    			if( internalHashLen != len2 ) changes = 1;
    			for( int i = 0; i < internalHashLen; i++ )
        		{
        			if( internalHash[ i ] != buf2[ i ] ) changes = 1;
        		}
    			h2.close();
    		} catch( Exception e ) { 
    	    	Log.e( "CopyResources", "Compare hashes", e ); 
    	    }
    	}
    	
    	if( changes == 0 )
    	{
    		Log.i( "CopyResources", "All files in place" );
    	}
    	
    	return changes;
	}
	
	public int UnpackAppResources( InputStream internalResourcesStream )
	{
		String externalFilesDir = GetDir( "external_files" );
		
       	//Copy internal hash to external:
       	try {
			FileOutputStream fout = new FileOutputStream( externalFilesDir + "/hash" );
			fout.write( internalHash, 0, internalHashLen );
			fout.close();
		} catch( Exception e ) { 
	    	Log.e( "CopyResources", "Write hash (external)", e ); 
	    }

		//Unpack all files:
       	BufferedInputStream zipInputStream = new BufferedInputStream( internalResourcesStream );
		String unzipLocation = externalFilesDir + "/";
		UnZip( zipInputStream, unzipLocation );
		
		return 0;
	}
}
