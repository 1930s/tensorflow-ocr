using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Pathing
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void btnOpen_Click(object sender, EventArgs e)
        {
            DialogResult result = openFileDialog1.ShowDialog(this);
            if (result == DialogResult.OK) // Test result.
            {
                try
                {
                    Image img = Image.FromFile(openFileDialog1.FileName);
                    pictureBox1.Image = img;
                    pictureBox1.Refresh();
                }
                catch (Exception err)
                {
                    MessageBox.Show("Error reading image: " + err.Message);
                }
            }

        }

        private void btnPlotPath_Click(object sender, EventArgs e)
        {
            //vertical plot
            Bitmap img = (Bitmap)pictureBox1.Image;
            int mid_x = Convert.ToInt32(txtStartX.Text);
            int cur_y = 0;
            int[] x_ar = new int[img.Height];
            x_ar[0] = mid_x;
            Color c, c1;

            //while (cur_y != img.Height)
            for (int x = 0; x < 1000; x++)
            {
                //cur_y is the index in the array, x_ar[cur_y] is the last point in the path

                pictureBox1.Image = img;
                pictureBox1.Refresh();

                //check down, if ok, go down
                if (cur_y + 1 == img.Height)
                {
                    //we are done, path found
                    break;
                }
                c = img.GetPixel(x_ar[cur_y], cur_y + 1);
                if ((c.R == 255) && (c.G == 255) && (c.B == 255))
                {
                    x_ar[cur_y + 1] = x_ar[cur_y];
                    img.SetPixel(x_ar[cur_y], cur_y + 1, Color.Green);
                    cur_y++;
                    continue;
                }
                //check down left, if ok, go down left
                //need to check 1 down, 1 left and 1 left to make sure we aren't scooting through a corner case
                c = img.GetPixel(x_ar[cur_y] - 1, cur_y + 1);
                c1 = img.GetPixel(x_ar[cur_y] - 1, cur_y);
                if ((c.R == 255) && (c.G == 255) && (c.B == 255) && (c1.R == 255) && (c1.G == 255) && (c1.B == 255))
                {
                    x_ar[cur_y + 1] = x_ar[cur_y] - 1;
                    img.SetPixel(x_ar[cur_y] - 1, cur_y + 1, Color.Green);
                    cur_y++;
                    continue;
                }
                //check down right, if ok, go down right
                //need to check 1 down, 1 right and 1 right to make sure we aren't scooting through a corner case
                c = img.GetPixel(x_ar[cur_y] + 1, cur_y + 1);
                c1 = img.GetPixel(x_ar[cur_y] + 1, cur_y);
                if ((c.R == 255) && (c.G == 255) && (c.B == 255) && (c1.R == 255) && (c1.G == 255) && (c1.B == 255))
                {
                    x_ar[cur_y + 1] = x_ar[cur_y] + 1;
                    img.SetPixel(x_ar[cur_y] + 1, cur_y + 1, Color.Green);
                    cur_y++;
                    continue;
                }
                //if we get here, we need to go either left or right
                //check right
                c = img.GetPixel(x_ar[cur_y] + 1, cur_y);
                if ((c.R == 255) && (c.G == 255) && (c.B == 255))
                {
                    x_ar[cur_y] = x_ar[cur_y] + 1;
                    img.SetPixel(x_ar[cur_y], cur_y, Color.Green);
                    continue;
                }
                //check left
                c = img.GetPixel(x_ar[cur_y] - 1, cur_y);
                if ((c.R == 255) && (c.G == 255) && (c.B == 255))
                {
                    x_ar[cur_y] = x_ar[cur_y] - 1;
                    img.SetPixel(x_ar[cur_y], cur_y, Color.Green);
                    continue;
                }
                //if we get here, we must have reached a blocking point where we need to backtrack
                //there can be three cases:
                //1) we are back to cur_y == 0, meaning there is no path
                //2) we are blocked on the right and have not tried to go left
                //3) we have tried both left and right and need to backtrack
                //case 1
                if (cur_y == 0) //we are finished, there is no path
                {
                    break;
                }
                //case 2
                //we get the original x value from the previous y index
                //we check to see if we've been/can go left
                c = img.GetPixel(x_ar[cur_y - 1] - 1, cur_y);
                if ((c.R == 255) && (c.G == 255) && (c.B == 255))
                {
                    //we can go left, so we reset this point
                    x_ar[cur_y] = x_ar[cur_y - 1];
                    continue;
                }
                else
                {
                    //case 3
                    cur_y--;
                }
            }
        }

        private void btnReset_Click(object sender, EventArgs e)
        {
            try
            {
                Image img = Image.FromFile(openFileDialog1.FileName);
                pictureBox1.Image = img;
                pictureBox1.Refresh();
            }
            catch (Exception err)
            {
                MessageBox.Show("Error reading image: " + err.Message);
            }

        }
    }
}
