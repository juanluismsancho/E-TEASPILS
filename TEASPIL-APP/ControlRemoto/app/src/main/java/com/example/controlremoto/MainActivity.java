package com.example.controlremoto;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.UUID;


public class MainActivity extends AppCompatActivity {

    EditText edtTextoOut;
    Button btnEnviar, btnLeer;
    TextView tvtMensaje;
    Button btnDesconectar;

    //-------------------------------------------
    Handler bluetoothIn;
    final int handlerState = 0;
    private BluetoothAdapter btAdapter = null;
    private BluetoothSocket btSocket = null;
    private StringBuilder DataStringIN = new StringBuilder();
    private ConnectedThread MyConexionBT;
    // Identificador unico de servicio - SPP UUID
    private static final UUID BTMODULEUUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    // String para la direccion MAC
    private static String address = null;
    //-------------------------------------------

    private static final String FILE_NAME = "texto.txt";
    private static final String carpeta = "/log/";
    private String archivo = "miarchivo";
    String file_path="";
    File file;
    String name="";

    String mensaje;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        //this.file_path=(Environment.getExternalStorageDirectory()+this.carpeta);
        this.file_path=(getFilesDir()+this.carpeta);
        File local_file=new File(this.file_path);

        if(!local_file.exists())
            local_file.mkdir();
        this.name = "miarchivo.txt";

        this.file = new File(local_file, this.name);

        try {
            this.file.createNewFile();
        } catch (IOException e) {
            e.printStackTrace();
        }

        bluetoothIn = new Handler() {
            public void handleMessage(android.os.Message msg) {
                if (msg.what == handlerState) {

                    char MyCaracter = (char) msg.obj;

                    if (MyCaracter != '1')
                    {
                        mensaje += MyCaracter;
                        tvtMensaje.setText("RECIBIENDO" + mensaje);
                    }else
                        saveFile(mensaje);
                }
            }
        };

        btAdapter = BluetoothAdapter.getDefaultAdapter(); // get Bluetooth adapter
        VerificarEstadoBT();

        btnEnviar = findViewById(R.id.btnEnviar);
        btnLeer = findViewById(R.id.btnLeer);
        tvtMensaje = findViewById(R.id.tvtMensaje);
        btnDesconectar = findViewById(R.id.btnDesconectar);

        btnEnviar.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String GetDat = "1";
                MyConexionBT.write(GetDat);
            }
        });

        btnLeer.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                readFile();
            }
        });

        btnDesconectar.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (btSocket!=null)
                {
                    try {btSocket.close();
                }
                catch (IOException e)
                    { Toast.makeText(getBaseContext(), "Error", Toast.LENGTH_SHORT).show();;}
                }
                finish();
            }
        });
    }

    private BluetoothSocket createBluetoothSocket(BluetoothDevice device) throws IOException
    {
        //crea un conexion de salida segura para el dispositivo usando el servicio UUID
        return device.createRfcommSocketToServiceRecord(BTMODULEUUID);
    }

    @Override
    public void onResume()
    {
        super.onResume();

        Intent intent = getIntent();
        address = intent.getStringExtra(DispositivosVinculados.EXTRA_DEVICE_ADDRESS);
        //Setea la direccion MAC
        BluetoothDevice device = btAdapter.getRemoteDevice(address);

        try
        {
            btSocket = createBluetoothSocket(device);
        } catch (IOException e) {
            Toast.makeText(getBaseContext(), "La creacción del Socket fallo", Toast.LENGTH_LONG).show();
        }
        // Establece la conexión con el socket Bluetooth.
        try
        {
            btSocket.connect();
        } catch (IOException e) {
            try {
                btSocket.close();
            } catch (IOException e2) {}
        }
        MyConexionBT = new ConnectedThread(btSocket);
        MyConexionBT.start();
    }

    @Override
    public void onPause()
    {
        super.onPause();
        try
        { // Cuando se sale de la aplicación esta parte permite que no se deje abierto el socket
            btSocket.close();
        } catch (IOException e2) {}
    }

    //Comprueba que el dispositivo Bluetooth
    //está disponible y solicita que se active si está desactivado
    private void VerificarEstadoBT() {

        if(btAdapter==null) {
            Toast.makeText(getBaseContext(), "El dispositivo no soporta bluetooth", Toast.LENGTH_LONG).show();
        } else {
            if (btAdapter.isEnabled()) {
            } else {
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                startActivityForResult(enableBtIntent, 1);
            }
        }
    }

    //Crea la clase que permite crear el evento de conexion
    private class ConnectedThread extends Thread
    {
        private final InputStream mmInStream;
        private final OutputStream mmOutStream;

        public ConnectedThread(BluetoothSocket socket)
        {
            InputStream tmpIn = null;
            OutputStream tmpOut = null;
            try
            {
                tmpIn = socket.getInputStream();
                tmpOut = socket.getOutputStream();
            } catch (IOException e) { }
            mmInStream = tmpIn;
            mmOutStream = tmpOut;
        }

        public void run()
        {
            byte[] byte_in = new byte[1];
            // Se mantiene en modo escucha para determinar el ingreso de datos
            while (true) {
                try {
                    mmInStream.read(byte_in);
                    char ch = (char) byte_in[0];
                    bluetoothIn.obtainMessage(handlerState, ch).sendToTarget();
                } catch (IOException e) {
                    break;
                }
            }
        }

        //Envio de trama
        public void write(String input)
        {
            try {
                mmOutStream.write(input.getBytes());
            }
            catch (IOException e)
            {
                //si no es posible enviar datos se cierra la conexión
                Toast.makeText(getBaseContext(), "La Conexión fallo", Toast.LENGTH_LONG).show();
                finish();
            }
        }
    }
    private void saveFile(String aux){

        FileWriter fichero = null;
        PrintWriter pw;
        //Toast toast = Toast.makeText(getApplicationContext(),"Entro a save", Toast.LENGTH_SHORT);
        //toast.show();
        try {
            Toast toast4 = Toast.makeText(getApplicationContext(),"AL MENOS ENTRO: "+  getFilesDir(), Toast.LENGTH_SHORT);
            toast4.show();
            fichero = new FileWriter(file);
            Toast toast1 = Toast.makeText(getApplicationContext(),"SE ha guardado 1", Toast.LENGTH_SHORT);
            toast1.show();
            pw = new PrintWriter(fichero);
            pw.println(aux);
            pw.flush();
            Toast toast2 = Toast.makeText(getApplicationContext(),"SE ha guardado 2", Toast.LENGTH_SHORT);
            toast2.show();
            pw.close();
            Toast toast3 = Toast.makeText(getApplicationContext(),"SE ha guardado 3", Toast.LENGTH_SHORT);
            toast3.show();
        }catch (Exception e){
            e.printStackTrace();
        }finally {
            try {
                if(null != fichero) {
                    fichero.close();
                    Toast toast2 = Toast.makeText(getApplicationContext(),"SALIIII", Toast.LENGTH_LONG);
                    toast2.show();
                }
            }catch (Exception e2)
            {
                e2.printStackTrace();
            }
        }
    }

    private void readFile(){
        String contenido = null;
        FileInputStream fileInputStream = null;
        try {
            fileInputStream = new FileInputStream(file);
        }catch (FileNotFoundException e){
            e.printStackTrace();
        }
        InputStreamReader inputStreamReader = new InputStreamReader(fileInputStream);
        BufferedReader bufferedReader = new BufferedReader(inputStreamReader);
        int ascii;

        try{
            String lineaTexto;
            StringBuilder stringBuilder = new StringBuilder();
            while((lineaTexto = bufferedReader.readLine())!=null){
                stringBuilder.append(lineaTexto).append("\n");
            }
            tvtMensaje.setText(stringBuilder);
        }catch (Exception e){
            e.printStackTrace();
        }finally {
            if(fileInputStream !=null){
                try {
                    fileInputStream.close();
                }catch (Exception e){

                }
            }
        }

    }
}