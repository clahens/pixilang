package nightradio.pixilang;

import android.app.NativeActivity;
import android.os.Bundle;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileInputStream;

import nightradio.androidlib.AndroidLib;

public class MyNativeActivity extends NativeActivity 
{
	AndroidLib lib;
	
    public MyNativeActivity() 
    {
    	super();
    	lib = new AndroidLib( this );
    }
    
    @Override
    protected void onCreate( Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );
    }

    public int CopyResources()
    {
    	int rv = lib.CheckAppResources( getResources().openRawResource( R.raw.hash ) );
    	if( rv > 0 )
    	{
    		rv = lib.UnpackAppResources( getResources().openRawResource( R.raw.files ) );
    	}
    	return rv;
    }
}